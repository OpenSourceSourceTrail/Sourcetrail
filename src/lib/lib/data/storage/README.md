# Datalayer

## Current implemenation

```mermaid
classDiagram
    class Storage {
        <<interface>>
        +setup() bool
        +clear()
        +beginTransaction()
        +commitTransaction()
        +rollbackTransaction()
        +isEmpty() bool
        +getVersion() StorageVersion
    }

    class StorageAccess {
        <<interface>>
        +getStorageStats() StorageStats
        +getNodeCount() size_t
        +getEdgeCount() size_t
        +getFileCount() size_t
    }

    class StorageAccessProxy {
        -m_storage: Storage*
        +setup() bool
        +clear()
        +isEmpty() bool
        +setVersion(version: StorageVersion)
    }

    class StorageCache {
        -m_nodeIdToNameCache: Map~Id, string~
        -m_fileNodeIdToPathCache: Map~Id, FilePath~
        +setupCaches()
        +clearCaches()
        +getAllNodeIds() vector~Id~
    }

    class PersistentStorage {
        -m_sqliteStorage: SqliteStorage
        -m_sourceLocationCollection: SourceLocationCollection
        +setupTables() bool
        +setupIndices() bool
        +buildCaches()
    }

    class SqliteStorage {
        -m_dbPath: FilePath
        +init() bool
        +execute(statement: string)
        +executeQuery(query: string)
        +vacuum()
    }

    class SqliteIndexStorage {
        -m_nodeIdToNameMap: Map~Id, string~
        -m_fileNodeIds: Set~Id~
        +addNode(node: StorageNode)
        +addSymbol(symbol: StorageSymbol)
        +addFile(file: StorageFile)
    }

    class SqliteBookmarkStorage {
        -m_bookmarkCategories: vector~BookmarkCategory~
        +addBookmarkCategory(category: BookmarkCategory)
        +addBookmark(bookmark: Bookmark)
        +getAllBookmarks() vector~Bookmark~
    }

    class IntermediateStorage {
        -m_nodes: vector~StorageNode~
        -m_edges: vector~StorageEdge~
        +getNodeCount() size_t
        +getEdgeCount() size_t
        +insertNode(node: StorageNode)
    }

    class StorageStats {
        +nodeCount: size_t
        +edgeCount: size_t
        +fileCount: size_t
        +completedFileCount: size_t
    }

    class StorageNode {
        +id: Id
        +type: NodeType
        +name: string
        +symbolType: SymbolType
    }

    class StorageEdge {
        +id: Id
        +type: EdgeType
        +sourceNodeId: Id
        +targetNodeId: Id
    }

    Storage <|.. PersistentStorage
    Storage <|.. IntermediateStorage
    StorageAccess <|.. StorageAccessProxy
    StorageAccessProxy <|-- StorageCache
    PersistentStorage *-- SqliteStorage
    SqliteStorage <|-- SqliteIndexStorage
    SqliteStorage <|-- SqliteBookmarkStorage
    StorageAccessProxy --> Storage
    PersistentStorage --> StorageStats
    SqliteIndexStorage --> StorageNode
    SqliteIndexStorage --> StorageEdge
```

## Future implemenation

```mermaid
classDiagram
    %% Core Domain Interfaces
    class ISourceStorage {
        <<interface>>
        +initialize(): Result~void~
        +shutdown(): void
        +getVersion(): Version
        +getHealth(): StorageHealth
    }

    class ISourceRepository~T~ {
        <<interface>>
        +find(query: Query): Stream~T~
        +save(entity: T): Result~T~
        +update(entity: T): Result~T~
        +remove(id: ID): Result~void~
        +exists(id: ID): bool
    }

    %% Domain Entities
    class CodeEntity {
        <<abstract>>
        +id: UUID
        +type: EntityType
        +metadata: Metadata
        +timestamp: DateTime
    }

    class SymbolEntity {
        +name: string
        +symbolType: SymbolType
        +scope: Scope
        +location: SourceLocation
    }

    class ReferenceEntity {
        +sourceId: UUID
        +targetId: UUID
        +type: ReferenceType
        +context: Context
    }

    %% Storage Implementation
    class StorageEngine {
        -config: StorageConfig
        -healthMonitor: HealthMonitor
        +createSession(): Session
        +backup(): Future~void~
        +optimize(): Future~void~
    }

    class StorageSession {
        -transactionManager: TransactionManager
        +beginTransaction(): Transaction
        +commit(): Result~void~
        +rollback(): void
    }

    %% Caching Layer
    class CacheManager {
        -policies: CachePolicies
        -metrics: CacheMetrics
        +get~T~(key: CacheKey): Option~T~
        +invalidate(pattern: Pattern)
    }

    %% Index Management
    class IndexManager {
        -indexStrategy: IndexStrategy
        -scheduler: IndexScheduler
        +buildIndex(): Future~void~
        +searchIndex(query: Query): Stream~Result~
    }

    %% Query Engine
    class QueryEngine {
        -optimizer: QueryOptimizer
        -executor: QueryExecutor
        +execute(query: Query): Stream~Result~
        +explain(query: Query): QueryPlan
    }

    %% Monitoring & Metrics
    class StorageMetrics {
        +collectMetrics(): Metrics
        +getHealth(): Health
        +getPerformance(): Performance
    }

    %% Repository Implementations
    class SymbolRepository {
        -queryEngine: QueryEngine
        -cacheManager: CacheManager
        +findByName(name: string): Stream~Symbol~
        +findByLocation(loc: Location): Symbol
    }

    class ReferenceRepository {
        -queryEngine: QueryEngine
        -indexManager: IndexManager
        +findBySymbol(symbolId: UUID): Stream~Reference~
        +findByType(type: ReferenceType): Stream~Reference~
    }

    %% Relationships
    ISourceStorage <|.. StorageEngine
    ISourceRepository <|.. SymbolRepository
    ISourceRepository <|.. ReferenceRepository
    CodeEntity <|-- SymbolEntity
    CodeEntity <|-- ReferenceEntity
    StorageEngine *-- StorageSession
    StorageEngine *-- CacheManager
    StorageEngine *-- IndexManager
    StorageEngine *-- QueryEngine
    StorageEngine *-- StorageMetrics
    SymbolRepository --> CacheManager
    SymbolRepository --> QueryEngine
    ReferenceRepository --> IndexManager
    ReferenceRepository --> QueryEngine
```


## Sourcetrail Storage Migration Roadmap

### Phase 1: Foundation Setup (4-6 weeks)

#### 1.1 Core Infrastructure

- [ ] Set up new project structure for the optimized storage system
- [ ] Implement base interfaces (ISourceStorage, ISourceRepository)
- [ ] Create core domain entities (CodeEntity, SymbolEntity, ReferenceEntity)
- [ ] Implement Result and Stream utility classes
- [ ] Set up comprehensive testing framework

#### 1.2 Basic Storage Operations

- [ ] Implement basic StorageEngine functionality
- [ ] Create Session management system
- [ ] Implement fundamental transaction handling
- [ ] Set up basic error handling infrastructure

#### 1.3 Initial Testing & Validation

- [ ] Create test suite for core components
- [ ] Validate basic storage operations
- [ ] Performance benchmarking of basic operations
- [ ] Document initial architecture

### Phase 2: Advanced Features Implementation (6-8 weeks)

#### 2.1 Query Engine

- [ ] Implement QueryEngine core
- [ ] Create query optimization system
- [ ] Develop query execution engine
- [ ] Add query caching mechanism

#### 2.2 Caching System

- [ ] Implement CacheManager
- [ ] Add cache policies (LRU, LFU)
- [ ] Implement cache metrics
- [ ] Create cache invalidation strategies

#### 2.3 Index Management

- [ ] Implement IndexManager
- [ ] Create indexing strategies
- [ ] Add index optimization
- [ ] Implement search functionality

### Phase 3: Migration Tools & Bridge (4-6 weeks)

#### 3.1 Data Migration Tools

- [ ] Create data migration scripts
- [ ] Implement format converters
- [ ] Add validation tools
- [ ] Create rollback mechanisms

#### 3.2 Legacy Bridge

- [ ] Implement adapter layer for legacy code
- [ ] Create compatibility layer
- [ ] Add legacy API support
- [ ] Implement feature flags system

### Phase 4: Gradual Migration (8-12 weeks)

#### 4.1 Component Migration

```mermaid
gantt
    title Storage Component Migration
    dateFormat  YYYY-MM-DD
    section Core
    Basic Storage    :2025-01-15, 30d
    Query Engine     :2025-02-15, 30d
    section Features
    Caching System   :2025-03-15, 30d
    Indexing        :2025-04-15, 30d
    section Integration
    Legacy Bridge   :2025-05-15, 30d
    Final Testing   :2025-06-15, 30d
```

#### 4.2 Migration Steps

1. Symbol Storage
   - Migrate basic symbol storage
   - Add new symbol features
   - Validate symbol operations
   - Performance testing

2. Reference Storage
   - Migrate reference handling
   - Implement new reference features
   - Cross-reference validation
   - Integration testing

3. Index Storage
   - Migrate index structures
   - Implement new indexing
   - Validate search operations
   - Performance optimization

### Phase 5: Testing & Optimization (4-6 weeks)

#### 5.1 Comprehensive Testing

- [ ] Unit tests for all components
- [ ] Integration tests
- [ ] Performance tests
- [ ] Migration tests
- [ ] Rollback tests

#### 5.2 Performance Optimization

- [ ] Query optimization
- [ ] Cache tuning
- [ ] Index optimization
- [ ] Memory usage optimization

### Phase 6: Deployment & Monitoring (4-6 weeks)

#### 6.1 Deployment Strategy

- [ ] Create deployment plans
- [ ] Set up monitoring
- [ ] Implement health checks
- [ ] Create rollback procedures

#### 6.2 Documentation & Training

- [ ] Technical documentation
- [ ] API documentation
- [ ] Migration guides
- [ ] Best practices

### Risk Mitigation Strategies

1. **Data Integrity**
   - Regular validation checks
   - Automated testing
   - Rollback capabilities
   - Data backup procedures

2. **Performance**
   - Regular benchmarking
   - Performance monitoring
   - Optimization cycles
   - Load testing

3. **Compatibility**
   - Legacy support
   - API versioning
   - Feature flags
   - Gradual rollout

### Success Metrics

- Zero data loss during migration
- Performance improvement > 30%
- Query response time reduction > 40%
- Memory usage reduction > 25%
- Successful migration of all existing features

### Timeline Summary

- Total Duration: 30-44 weeks
- Critical Path: Core Implementation → Migration Tools → Gradual Migration
- Buffer: 4-6 weeks for unexpected issues
