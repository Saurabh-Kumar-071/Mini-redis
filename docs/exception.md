TOPIC: EXCEPTION HIERARCHY

Goal:
Categorize errors.

Hierarchy:

std::exception
      ↑
runtime_error
      ↑
MiniRedisException
      ↑
 ├── SocketException
 ├── PersistenceException
 └── DatabaseException

Benefits:

✓ Better debugging
✓ Error categorization
✓ Inheritance practice
✓ Cleaner code
✓ Future scalability

**** explicit = Stop automatic object creation/conversion.******
***********Exception handling flow ***********

Current function stops
        ↓
Stack unwinding starts
        ↓
Search for matching catch
        ↓
main() catches it
