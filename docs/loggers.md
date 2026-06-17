
*******************************(Mini Redis Logger Preparation)*******************************
ILogger& logger means Borrowed dependency.
unique_ptr<ILogger> meansOwned dependency.

Constructor acquires resources.
Destructor releases resources.

Polymorphism means:
Same function call,
Different behavior depending on the actual object.


*********dependency injection*********
Server depends on ILogger.
Server does not own ILogger.
Server receives ILogger from outside.
*****************************************

********Mini Redis Logger Preparation & Polymorphism Notes********

1. ABSTRACT CLASS

---

Example:

class ILogger
{
public:
virtual void info(const string& message) = 0;
virtual void error(const string& message) = 0;
};

A class containing at least one pure virtual function (= 0) becomes an Abstract Class.

Properties:

* Cannot create objects.
* Used as a blueprint/interface.
* Derived classes must implement all pure virtual functions.

Example:

ILogger logger;   ❌ Not Allowed

Reason:
ILogger is incomplete.

2. PURE VIRTUAL FUNCTION

---

Example:

virtual void info(const string& message) = 0;

Meaning:

* No implementation exists in base class.
* Derived classes must provide implementation.

Example:

class ConsoleLogger : public ILogger
{
public:
void info(const string& message) override
{
cout << message;
}
};

3. POLYMORPHISM

---

Meaning:

Same interface,
Different behavior depending on actual object.

Example:

Animal* animal;

animal = &dog;
animal->sound();   // Bark

animal = &cat;
animal->sound();   // Meow

The function call is identical:

animal->sound();

But behavior changes according to actual object.

4. STATIC BINDING

---

Definition:

Function call resolved at Compile Time.

Example:

Dog dog;
dog.sound();

Compiler already knows:

dog is Dog

Therefore:

Dog::sound()

is selected before execution.

Also called:

* Early Binding
* Compile Time Binding

5. DYNAMIC BINDING

---

Definition:

Function call resolved at Runtime.

Requires:

virtual function

Example:

Animal* animal = &dog;

animal->sound();

Compiler only knows:

Animal*

Actual object:

Dog

At runtime:

Dog::sound()

is selected.

Also called:

* Runtime Binding
* Late Binding

6. WHY VIRTUAL IS NEEDED

---

Without virtual:

Animal* animal = &dog;
animal->sound();

Output:

Animal

Because compiler looks at pointer type.

With virtual:

Animal* animal = &dog;
animal->sound();

Output:

Bark

Because runtime looks at actual object.

7. LOGGER DESIGN

---

Current:

cout << "Client Connected";

Future:

logger->info("Client Connected");

Benefits:

* Can change logger implementation easily.
* Server does not care where logs go.

Possible implementations:

ILogger
|
+-- ConsoleLogger
+-- FileLogger
+-- NetworkLogger

8. LOGGER POLYMORPHISM

---

ILogger* logger;

logger = new ConsoleLogger();
logger->info("Connected");

OR

logger = new FileLogger();
logger->info("Connected");

Same function call:

logger->info()

Different behavior.

This is real-world polymorphism.

9. IMPORTANT RULE

---

If you have:

Base* ptr = new Derived();

and want:

Derived::function()

to execute,

then:

function()

must be virtual in Base class.

10. CURRENT MINI REDIS STATUS

---

Completed:

✓ TCP Socket Server
✓ Epoll
✓ Non-blocking I/O
✓ Database
✓ Parser
✓ Executor
✓ ConnectionManager
✓ ClientConnection
✓ Persistence
✓ Exception System

Next:

→ Logger
→ Virtual Destructor
→ ConsoleLogger
→ Dependency Injection
→ Command Pattern
→ TTL Expiry
→ Thread Pool

## INTERVIEW DEFINITIONS

Polymorphism:
"Same interface, different behavior."

Static Binding:
"Function call resolved at compile time."

Dynamic Binding:
"Function call resolved at runtime using virtual functions."

Abstract Class:
"A class containing at least one pure virtual function and cannot be instantiated."


TOPIC: DEPENDENCY INJECTION

Bad:

Server creates ConsoleLogger itself.

Problem:

Server is tightly coupled
to ConsoleLogger.

Good:

ConsoleLogger logger;

Server server(logger);

Benefits:

✓ Loose Coupling
✓ Easy Testing
✓ Supports Polymorphism
✓ Easily replace ConsoleLogger
  with FileLogger
