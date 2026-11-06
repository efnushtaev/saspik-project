# Bridge Pattern - Mermaid Diagram

```mermaid
classDiagram
    class Client {
    }
    
    class Abstraction {
        <<abstract>>
        -Implementor implementor
        +Operation()
    }
    
    class RefinedAbstraction {
        -Implementor implementor
        +Operation()
    }
    
    class Implementor {
        <<interface>>
        +OperationImp()
    }
    
    class ConcreteImplementorA {
        +OperationImp()
    }
    
    class ConcreteImplementorB {
        +OperationImp()
    }
    
    Client --> Abstraction : uses
    Abstraction *-- Implementor : implementor
    RefinedAbstraction --|> Abstraction : inherits
    ConcreteImplementorA --|> Implementor : inherits
    ConcreteImplementorB --|> Implementor : inherits
    Abstraction ..> Implementor : calls implementor.OperationImp()