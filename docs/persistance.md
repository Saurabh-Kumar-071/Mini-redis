**********************************************************************************************************************************
**********************************************************************************************************************************
                                                  TOPIC: READ-ONLY ACCESS
**********************************************************************************************************************************
**********************************************************************************************************************************

Problem:
PersistenceManager needs to read
database contents.

Database data is private.

Bad Solution:
Make data public.

Good Solution:
Expose read-only access through a getter.

Method:

const unordered_map<string,string>&
getAllData() const;

Benefits:

✓ Encapsulation preserved
✓ No copying
✓ Read-only access
✓ Better architecture

Concepts Learned:

- const correctness
- reference return
- encapsulation
- read-only views

TOPIC: FORWARD DECLARATION

Syntax:

class Database;

Purpose:

Tell compiler a class exists
without including its header.

Benefits:

✓ Faster compilation
✓ Fewer dependencies
✓ Cleaner architecture

Persistence Design:

save(const Database&)

Read-only access.

load(Database&)

Read + modify access.

Concepts Learned:

- Forward Declaration
- Dependency Reduction
- Read-only references
- Mutable references

*********************************************************************************************************************************

Forward Declaration Rule

Use:

class Database;

when storing:
- Database*
- Database&
- function parameters
- function return types

Use:

#include "Database.h"

when:
- creating a Database object
- inheriting from Database
- accessing Database members in header files

Reason:

Pointers/references have known size.
Objects require full class definition.


*********************************************************************************************************************************

====================================================
TOPIC: PERSISTENCE INTEGRATION
====================================================

CommandExecutor needs:

- Database&
- PersistenceManager&

Reason:

Executor modifies data.

The class that modifies data
should trigger persistence.

SET:

db.set(...)
persistence.save(db)

DEL:

db.del(...)
persistence.save(db)

Benefits:

✓ Automatic saving
✓ No data loss after commands
✓ Cleaner architecture
✓ Single Responsibility Principle

Ownership:

Server owns:
- Database
- PersistenceManager

CommandExecutor borrows:
- Database&
- PersistenceManager&

====================================================
