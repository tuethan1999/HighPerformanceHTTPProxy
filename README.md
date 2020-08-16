# HighPerformanceHTTPProxy

The purpose of this project was to create a robust high performance proxy with the following features:
- HTTP caching
- CONNECT tunneling(HTTPS)
- Handling partial messages
- Rate limiting via the bucket method

### Modules
File | Functionality
------------ | -------------
proxy.c| Proxy Implementation-- all files moved into here for performance and grading
Bucket.c/h   | Leaky/Token Bucket algorithm to limit and regulate network
Buffer.c/h.  | Stores partial messages in a buffer so complete packets can be handled
CacheObject.c/h| Object that is stored within the cache
cache_tests.c| Tests CacheObject
HandleMessage.c/h| Handles messages from clients and servers and stores partial messages in a buffer array
HttpCache.c/h| Store and query and delete cache objects
RequestParser.c/h| Parse a HTTP requests by taking in a buffer and storing it in struct which has values for every possible optional argument
ServerHandler.c/h| Communicates with servers and keeps track of which servers we're talking to
baseline_unit_tests.c/h| Unit tests for baseline functionality without Rate limiting
utarray.h| dynamic array implementation
uthash.h| hash table implementation

### Testing
We can see that we get relatively stable performance with fixed number of clients but we have decreasing performance with more clients. This shows how rate limiting allows us to manage performance against greedy clients but is still susceptible to a denial of service style attack

##### Performance with fixed requests per client and variable numbr of clients
![performance with fixed clients](/performance_tests/ten_requests.png)

##### Performance with fixed clients and variable number of requests per client
![performance with fixed clients](/performance_tests/ten_clients.png)
