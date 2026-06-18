MINI REDIS - TTL (TIME TO LIVE) ARCHITECTURE

==================================================

1. GOAL
   ==================================================

TTL (Time To Live) allows a key to expire automatically
after a specified amount of time.

Example:

SET otp 123456
EXPIRE otp 120

Meaning:

* Store key "otp"
* Delete it automatically after 120 seconds

==================================================
2. DATABASE DESIGN
==================

Main Storage:

unordered_map<string,string> data;

TTL Storage:

unordered_map<
string,
chrono::system_clock::time_point

> expiry;

Responsibilities:

data:
key -> value

expiry:
key -> expiration timestamp

Example:

data:

otp -> 123456

expiry:

otp -> 10:32:00

==================================================
3. WHY TWO MAPS?
================

Data and expiration metadata have different purposes.

data:

otp -> 123456

expiry:

otp -> 10:32:00

Benefits:

* Cleaner design
* Easy TTL lookup
* Similar to Redis internals

==================================================
4. EXPIRE COMMAND
=================

Command:

EXPIRE otp 120

Executor:

db.expire("otp",120);

Implementation:

expiry[key] =
chrono::system_clock::now()
+ chrono::seconds(seconds);

Example:

Current Time:

10:30:00

EXPIRE otp 120

Stored Timestamp:

10:32:00

==================================================
5. WHY STORE TIMESTAMP?
=======================

Wrong:

Store countdown

otp -> 120

Problem:

After restart we lose
how much time has passed.

Correct:

Store absolute expiration time

otp -> 10:32:00

Benefits:

* Survives restart
* Easy expiration check
* Easy persistence

==================================================
6. EXPIRATION CHECK
===================

Function:

bool isExpired(key)

Algorithm:

1. Check whether key has TTL

2. Get current time

3. Compare:

current_time >= expiry_time

4. Return result

Example:

Current Time:

10:33:00

Expiration Time:

10:32:00

Result:

Expired

==================================================
7. LAZY EXPIRATION
==================

Location:

Database::get()

Flow:

GET otp
↓
isExpired()
↓
Expired?
↓
YES
↓
Remove key
Return not-found

NO
↓
Return value

Benefits:

* Very simple
* No background thread
* Used by Redis

==================================================
8. TTL COMMAND
==============

Purpose:

Return remaining lifetime.

Command:

TTL otp

Possible Results:

-2
Key does not exist

-1
Key exists
No TTL

N
Remaining seconds

Example:

SET otp 123456

EXPIRE otp 60

TTL otp

Output:

59

==================================================
9. TTL CALCULATION
==================

Formula:

remaining_time =
expiration_time
---------------

current_time

Implementation:

auto remaining_time =
expiry[key]
-
chrono::system_clock::now();

return duration_cast<
chrono::seconds

> (
> remaining_time
> ).count();

==================================================
10. PERSISTENCE PROBLEM
=======================

Without TTL Persistence:

SET otp 123456

EXPIRE otp 120

Server Restart

Result:

otp survives forever

Problem:

TTL information lost.

==================================================
11. TTL PERSISTENCE DESIGN
==========================

File Format:

Normal Key:

name=saurabh

Key With TTL:

otp=123456|1719234000

Where:

1719234000

is expiration timestamp.

==================================================
12. SAVE PROCESS
================

Loop through:

data map

For each key:

IF key has TTL:

save:

key=value|timestamp

ELSE:

save:

key=value

Example:

name=saurabh

otp=123456|1719234000

==================================================
13. LOAD PROCESS
================

Read file line by line.

Case 1:

name=saurabh

Load:

db.set(
"name",
"saurabh"
);

Case 2:

otp=123456|1719234000

Load:

db.set(
"otp",
"123456"
);

db.setExpiryTime(
"otp",
timestamp
);

==================================================
14. WHY setExpiryTime()?
========================

expire()

Input:

seconds

Example:

expire("otp",120)

Converts:

120 seconds
↓
timestamp

Used by:

Client Commands

setExpiryTime()

Input:

timestamp

Example:

setExpiryTime(
"otp",
10:32:00
)

Stores timestamp directly.

Used by:

Persistence Loading

==================================================
15. SKIPPING EXPIRED KEYS
=========================

During Load:

if(
expiryTime
<=
chrono::system_clock::now()
)
{
continue;
}

Meaning:

Expired keys are not restored.

Example:

File:

otp=123456|old_timestamp

Result:

Ignored during load.

==================================================
16. COMPLETE FLOW
=================

SET otp 123456
↓

EXPIRE otp 120
↓

# expiry["otp"]

10:32:00

```
↓
```

SAVE

otp=123456|1719234000

```
↓
```

SERVER RESTART

```
↓
```

LOAD

```
↓
```

Restore value

Restore expiration timestamp

```
↓
```

TTL continues normally

==================================================
17. INTERVIEW QUESTIONS
=======================

Q1.
Why use two maps?

Answer:

Separate values from expiration metadata.

Q2.
Why store timestamp instead of countdown?

Answer:

Countdown cannot survive restart.
Timestamp can.

Q3.
What is Lazy Expiration?

Answer:

Expiration checked only when key is accessed.

Q4.
Why use system_clock instead of steady_clock?

Answer:

system_clock can be persisted and restored.
steady_clock cannot.

Q5.
Why create setExpiryTime()?

Answer:

expire() accepts seconds.

Persistence already knows the exact timestamp.

Therefore a separate function is needed.

==================================================
18. CURRENT STATUS
==================

COMPLETED

✓ EXPIRE command

✓ TTL command

✓ Expiry Map

✓ isExpired()

✓ Lazy Expiration

✓ TTL Persistence

✓ Save Expiration Timestamp

✓ Load Expiration Timestamp

✓ Skip Expired Keys During Load

✓ Restart Safe TTL

NEXT FEATURES

□ Active Expiration Thread

□ RESP Protocol

□ Command Pattern

□ Thread Pool

□ AOF Persistence
