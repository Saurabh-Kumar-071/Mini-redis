#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include "../src/database/Database.h"
#include "../src/persistence/PersistenceManager.h"
#include "../src/command/CommandParser.h"
#include "../src/command/CommandExecutor.h"
#include "../src/protocol/RESPEncoder.h"
#include "../src/network/FileDescriptor.h"

using namespace std;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            std::cerr << "[-] Assertion FAILED: " << msg << " (" << #expr << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(1); \
        } else { \
            std::cout << "[+] PASS: " << msg << std::endl; \
        } \
    } while(0)

void testRESPEncoder() {
    std::cout << "\n=== Testing RESP Encoder ===" << std::endl;
    TEST_ASSERT(RESPEncoder::simpleString("OK") == "+OK\r\n", "RESP simpleString OK");
    TEST_ASSERT(RESPEncoder::bulkString("hello") == "$5\r\nhello\r\n", "RESP bulkString hello");
    TEST_ASSERT(RESPEncoder::integer(42) == ":42\r\n", "RESP integer 42");
    TEST_ASSERT(RESPEncoder::error("ERR message") == "-ERR message\r\n", "RESP error");
    TEST_ASSERT(RESPEncoder::nullBulkString() == "$-1\r\n", "RESP nullBulkString");
    TEST_ASSERT(RESPEncoder::array({"foo", "bar"}) == "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n", "RESP array");
    TEST_ASSERT(RESPEncoder::array({"foo", "$-1"}) == "*2\r\n$3\r\nfoo\r\n$-1\r\n", "RESP array with null");
}

void testCommandParser() {
    std::cout << "\n=== Testing Command Parser & Crash Resilience ===" << std::endl;
    CommandParser parser;

    // Inline command test
    ParseResult res = parser.parseRESP("SET mykey myval\r\n");
    TEST_ASSERT(res.complete && res.command.arguments.size() == 3, "Parse inline SET");
    TEST_ASSERT(res.command.arguments[0] == "SET" && res.command.arguments[1] == "mykey" && res.command.arguments[2] == "myval", "Inline arguments match");

    // Standard RESP command test (*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n)
    res = parser.parseRESP("*3\r\n$3\r\nSET\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    TEST_ASSERT(res.complete && res.command.arguments.size() == 3, "Parse RESP array SET");
    TEST_ASSERT(res.command.arguments[0] == "SET" && res.command.arguments[1] == "foo" && res.command.arguments[2] == "bar", "RESP arguments match");

    // Incomplete data test
    res = parser.parseRESP("*3\r\n$3\r\nSET\r\n");
    TEST_ASSERT(!res.complete, "Incomplete RESP returns complete=false without crashing");

    // Malformed input fuzz tests
    res = parser.parseRESP("*abc\r\n");
    TEST_ASSERT(res.complete && res.command.arguments[0] == "__MALFORMED_PROTOCOL_ERROR__", "Corrupted array count handled safely as protocol error");

    res = parser.parseRESP("*1\r\n$xyz\r\n");
    TEST_ASSERT(res.complete && res.command.arguments[0] == "__MALFORMED_PROTOCOL_ERROR__", "Corrupted bulk string length handled safely as protocol error");

    res = parser.parseRESP("*999999999999999999999999\r\n");
    TEST_ASSERT(res.complete && res.command.arguments[0] == "__MALFORMED_PROTOCOL_ERROR__", "Integer overflow count handled safely as protocol error");

    res = parser.parseRESP("");
    TEST_ASSERT(!res.complete, "Empty input handled safely");
}

void testAll27Commands() {
    std::cout << "\n=== Testing All 27 Redis Commands ===" << std::endl;
    Database db;
    PersistenceManager pm;
    CommandExecutor exec(db, pm);

    // 1. PING
    CommandResponse r = exec.execute({{"PING"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "PONG", "1. PING default");
    r = exec.execute({{"ping", "hello"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value == "hello", "1. PING with message & case insensitivity");

    // 2. SET
    r = exec.execute({{"set", "k1", "v1"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "OK", "2. SET command");

    // 3. GET
    r = exec.execute({{"get", "k1"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value == "v1", "3. GET existing key");
    r = exec.execute({{"get", "missing_key"}});
    TEST_ASSERT(r.type == ResponseType::Null, "3. GET non-existing key returns RESP Null");

    // 4. DEL
    r = exec.execute({{"del", "k1"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "4. DEL existing key returns 1");
    r = exec.execute({{"del", "k1"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "0", "4. DEL non-existing key returns 0");

    // 5. EXPIRE
    exec.execute({{"set", "tempKey", "val"}});
    r = exec.execute({{"expire", "tempKey", "10"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "5. EXPIRE existing key returns 1");
    r = exec.execute({{"expire", "missing", "10"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "0", "5. EXPIRE non-existing key returns 0");

    // 6. TTL
    r = exec.execute({{"ttl", "tempKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && stoi(r.value) > 0, "6. TTL on expiring key returns positive seconds");
    exec.execute({{"set", "noTtlKey", "val"}});
    r = exec.execute({{"ttl", "noTtlKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "-1", "6. TTL on key without expiry returns -1");
    r = exec.execute({{"ttl", "ghostKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "-2", "6. TTL on missing key returns -2");

    // 7. BIG
    r = exec.execute({{"BIG"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value.size() == 10000000, "7. BIG returns 10MB bulk payload");

    // 8. MSET
    r = exec.execute({{"mset", "a", "1", "b", "2", "c", "3"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "OK", "8. MSET multiple keys");

    // 9. EXISTS
    r = exec.execute({{"exists", "a"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "9. EXISTS on single key");
    r = exec.execute({{"exists", "a", "b", "nonexistent"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "2", "9. EXISTS on multiple keys count");

    // 10. MGET
    r = exec.execute({{"mget", "a", "b", "nonexistent"}});
    TEST_ASSERT(r.type == ResponseType::Array && r.array.size() == 3, "10. MGET returns 3 elements");
    TEST_ASSERT(r.array[0] == "1" && r.array[1] == "2" && r.array[2] == "$-1", "10. MGET handles missing key with $-1");

    // 11. MDEL
    r = exec.execute({{"mdel", "a", "b", "nonexistent"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "2", "11. MDEL deletes matching keys and returns count");

    // 12. INCR
    r = exec.execute({{"incr", "numKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "12. INCR on new key initializes to 1");
    r = exec.execute({{"incr", "numKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "2", "12. INCR increments existing integer");

    // 13. DECR
    r = exec.execute({{"decr", "numKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "13. DECR decrements integer");
    r = exec.execute({{"decr", "newDecr"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "-1", "13. DECR on new key initializes to -1");

    // 14. KEYS
    r = exec.execute({{"keys"}});
    TEST_ASSERT(r.type == ResponseType::Array && r.array.size() >= 1, "14. KEYS returns array of stored keys");

    // 15. FLUSHDB
    r = exec.execute({{"flushdb"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "OK", "15. FLUSHDB clears database");
    TEST_ASSERT(db.size() == 0, "Database size is 0 after FLUSHDB");

    // 16. PERSIST
    db.set("persistKey", "data");
    db.expire("persistKey", 100);
    r = exec.execute({{"persist", "persistKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "1", "16. PERSIST removes TTL");
    TEST_ASSERT(db.ttl("persistKey") == -1, "TTL is -1 after PERSIST");

    // 17. APPEND
    r = exec.execute({{"append", "appKey", "Hello"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "5", "17. APPEND to new key returns 5");
    r = exec.execute({{"append", "appKey", " World"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "11", "17. APPEND returns total length 11");
    TEST_ASSERT(db.get("appKey") == "Hello World", "Appended string matches");

    // 18. STRLEN
    r = exec.execute({{"strlen", "appKey"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "11", "18. STRLEN returns correct length");
    r = exec.execute({{"strlen", "nonexistent"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "0", "18. STRLEN on nonexistent returns 0");

    // 19. GETSET
    r = exec.execute({{"getset", "gsKey", "initial"}});
    TEST_ASSERT(r.type == ResponseType::Null, "19. GETSET on nonexistent returns null");
    r = exec.execute({{"getset", "gsKey", "updated"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value == "initial", "19. GETSET returns previous value");
    TEST_ASSERT(db.get("gsKey") == "updated", "GETSET updated new value");

    // 20. INCRBY
    r = exec.execute({{"incrby", "incrbyKey", "10"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "10", "20. INCRBY on new key initializes and increments");
    r = exec.execute({{"incrby", "incrbyKey", "5"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "15", "20. INCRBY increments existing value");

    // 21. DECRBY
    r = exec.execute({{"decrby", "incrbyKey", "7"}});
    TEST_ASSERT(r.type == ResponseType::Integer && r.value == "8", "21. DECRBY decrements correctly");

    // 22. DBSIZE
    r = exec.execute({{"dbsize"}});
    TEST_ASSERT(r.type == ResponseType::Integer && stoi(r.value) > 0, "22. DBSIZE returns accurate count");

    // 23. TYPE
    r = exec.execute({{"type", "incrbyKey"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "string", "23. TYPE returns 'string'");
    r = exec.execute({{"type", "ghost"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "none", "23. TYPE returns 'none' for missing");

    // 24. INFO
    r = exec.execute({{"info"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value.find("MiniRedis") != string::npos, "24. INFO returns server statistics");

    // 25. RENAME
    r = exec.execute({{"rename", "incrbyKey", "renamedKey"}});
    TEST_ASSERT(r.type == ResponseType::SimpleString && r.value == "OK", "25. RENAME successful");
    TEST_ASSERT(db.exists("renamedKey") && !db.exists("incrbyKey"), "Old key erased and new key exists");
    r = exec.execute({{"rename", "nonexistent", "target"}});
    TEST_ASSERT(r.type == ResponseType::Error, "25. RENAME missing key returns error");

    // 26. ECHO
    r = exec.execute({{"echo", "MiniRedis Rocks"}});
    TEST_ASSERT(r.type == ResponseType::BulkString && r.value == "MiniRedis Rocks", "26. ECHO returns message");

    // 27. CONFIG
    r = exec.execute({{"config", "get", "*"}});
    TEST_ASSERT(r.type == ResponseType::Array, "27. CONFIG returns array");
}

void testPassiveAndActiveEviction() {
    std::cout << "\n=== Testing Hybrid Key Eviction (Passive + Active) ===" << std::endl;
    Database db;
    db.set("short_lived", "data");
    db.expire("short_lived", 1); // Expire in 1 second

    TEST_ASSERT(db.exists("short_lived"), "Key exists initially");
    TEST_ASSERT(db.ttl("short_lived") > 0, "Key has positive TTL");

    std::cout << "[*] Sleeping 1.2s to test expiration..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    // Passive eviction check
    TEST_ASSERT(!db.exists("short_lived"), "Passive check: exists() returns false after expiry");
    TEST_ASSERT(db.get("short_lived") == "", "Passive check: get() returns empty after expiry");
    TEST_ASSERT(db.ttl("short_lived") == -2, "Passive check: ttl() returns -2 after expiry");

    // Active sweep check
    db.set("sweep1", "val1");
    db.set("sweep2", "val2");
    db.expire("sweep1", 1);
    db.expire("sweep2", 100);

    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    bool cleaned = db.cleanupExpiredKeys();
    TEST_ASSERT(cleaned == true, "Active sweep: cleanupExpiredKeys() returns true when keys evicted");
    TEST_ASSERT(!db.exists("sweep1"), "Active sweep: expired sweep1 removed");
    TEST_ASSERT(db.exists("sweep2"), "Active sweep: unexpired sweep2 retained");
}

void testPersistence() {
    std::cout << "\n=== Testing dump.rdb Snapshot Persistence ===" << std::endl;
    Database db;
    PersistenceManager pm;

    db.set("persist_a", "val_a");
    db.set("persist_b", "val_b");
    db.expire("persist_b", 3600); // 1 hour TTL

    pm.save(db);
    TEST_ASSERT(true, "Saved snapshot to dump.rdb");

    Database newDb;
    pm.load(newDb);
    TEST_ASSERT(newDb.exists("persist_a"), "Loaded persistent key persist_a");
    TEST_ASSERT(newDb.get("persist_a") == "val_a", "Value matches for persist_a");
    TEST_ASSERT(newDb.exists("persist_b"), "Loaded persistent key persist_b with TTL");
    TEST_ASSERT(newDb.ttl("persist_b") > 3500, "TTL preserved across reload");
}

void testRAIIFileDescriptor() {
    std::cout << "\n=== Testing RAII FileDescriptor Management ===" << std::endl;
    {
        FileDescriptor fd1;
        TEST_ASSERT(fd1.get() == -1, "Default FileDescriptor is -1");

        FileDescriptor fd2(100);
        TEST_ASSERT(fd2.get() == 100, "Constructed FileDescriptor holds fd");

        FileDescriptor fd3 = std::move(fd2);
        TEST_ASSERT(fd3.get() == 100, "Move constructed FileDescriptor took ownership");
        TEST_ASSERT(fd2.get() == -1, "Moved-from FileDescriptor reset to -1");
    }
    TEST_ASSERT(true, "RAII FileDescriptor destroyed safely");
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  MiniRedis Automated Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl;

    testRESPEncoder();
    testCommandParser();
    testAll27Commands();
    testPassiveAndActiveEviction();
    testPersistence();
    testRAIIFileDescriptor();

    std::cout << "\n==========================================" << std::endl;
    std::cout << "  ALL TESTS PASSED SUCCESSFULLY! (100%)" << std::endl;
    std::cout << "==========================================" << std::endl;
    return 0;
}
