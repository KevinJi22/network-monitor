* a quiet route is a route that may not be the fastest but is noticeably less crowded than other routes
* we will build a client (network data) and a server (itinerary recommendations) using websockets and integrate them into the LTC infrastructure

#### Underground Network Layout

* network layout is described by a JSON file listing all times and stations
* `stations` is an array with all the underground stations
  * each station is uniquely identified by a `station_id`
  * each item in the array contains metadata about the station

* `lines` is an array with all the underground lines
  * each line is uniquely identified by a `line_id`
  * an underground line maps to one or more **routes**. A route is a specific path from station A to station B, in a specific direction. A line usually has two or more routes, e.g. A->B and B->A.
  * each `line` object contains a `route` array containing a list of routes
    * each `route` object has a `route_id`, a start and end station ID, and the direction

* `travel_times` is an array of objects indicating the time it takes to travel between two adjacent stations 
  * the distance is measured in minutes

#### Real-time network events

* the `network-events` data feed provides real-time network events for the lines.
  * passenger events: these represent a passenger action, like entering/exiting a station.

* the data feed follows the STOMP protocol and is served over secure WebSockets.
* A Websocket connection to the server is kept alive for a maximum of 60 seconds of idle activity from the client.
* the data feed requires all connections to be secured over TLS and authenticated via a key
* the API mimics real data - bursty data, data is produced every day

#### Quiet route recommendation service

* given a requst to travel from A to B, where A and B are underground stations, the `quiet-route` service provides a single itinerary recommendation that avoids busy lines and stations.
* all connections must be secured over TLS, so to connect, we have to send a STOMP connection frame

#### STOMP

* it only covers a small subset of commonly used messaging operations
* frame based protocol with frames modelled on HTTP
  * a frame consists of a command, a set of optional headers, and an optional body

* a STOMP client can be a producer or a consumer
* a WebSocket is a specification allowing bidirectional communication between a client and a server. It allows them to exchange variable length frames instead of a stream.
* STOMP defines a protocol for clients and servers to communicate.
  * it defines a few frame types that are mapped onto WebSockets frames, like connect, subscribe, unsubscribe, send, etc.

#### WebSockets

* WebSockets are another kind of application layer protocol that also use TCP to transport packets but are slightly different from HTTP in some aspects.
* WebSockets connections are long-lived because once a connection is established between client and server, there is no need for more handshakes to keep it alive. 
* HTTP either closes the connection after the message is sent or keeps it open but requires more handshakes. This causes additional overhead.
* WebSockets connections are **bi-directional** because both the client and server can send messages, and they are **full-duplex** because messages can be exchanged in both directions, at the same time, on the same channel.

#### Requirements

* what we need to build
  * client to interact with `network-events` data feed
    * this client must support TLS connections and needs to authenticate with the server.
  * `quiet-route` server. The server must support TLS connections but does not need authentication
  * a C++ object representing the network layout and the level of crowding, updating in response to passenger events.  
    * this also offers APIs to produce itineraries from A to B.
  * a C++ component that produces quiet route recommendations based on a request to go from A to B
* technologies we have to use, support, and integrate with
  * STOMP
  * WebSockets
  * TLS
  * JSON
* hard constraints
  * network serves up to **5 million** passenger journeys a day, so a total of *10 million* passenger events per day.
  * the underground operates from 5am to 12am, a total of 19 hours. This leads to an average of 0.5 million events per hour, or 150 events per second.
    * a conservative estimate is 300 events per second, which is an average of 3ms to handle a single event on a single threaded implementation

  * no hard memory requirements
    * don't need to store every passenger event that arrives

  * 15% of all trips are driven by a user search on services.
  * the quiet route feature will only be available to 20% of users at first, so 150k requests per day.
  * to be conservative, we will support 200k-400k requests per day, or 6 requests per second.
* soft constraints
  * good unit testing
  * integration and stress test
  * easy to maintain, need solid build system and automated deployment pipeline
* open questions

#### Boost Asynchronous operations

* I/O context is useful because you can leave it running in the background, interacting with low-level I/O interfaces, while the program works on something else
* when the I/O objects complete some work, they notify the I/O context, which notifies you
* Synchronous example for `socket.connect`:
  1. the `connect` function talks to the `io_context` object
  2. the `io_context` object talks to the TCP socket interface to connect to an endpoint
  3. when the connection completes or fails, the TCP socket replies directly the the I/O context, which receives the response
  4. the I/O context forwards the response back to us through the `error_code` variable.

* in the case of asynchronous calls, we pass a __callback handler__ to the function instead of receiving the result in the `error_code` argument.

**Note: when using the asynchronous APIs of an I/O object, we have to call `io_context::run` at least once, otherwise the asynchronous callbacks have no context in which to run.**

* the I/O context `run()` function returns when there is no work left to do.
* the `run()` function provides a context to run asynchronous callbacks for I/O objects. If using threads, the `run()` function runs in a different thread than `main`.
* this gives us control over **how many resources we give to the I/O context**.
* if you call the `run` functino from the _same_ `io_context` instance in multiple threads, then every thread will be an independent execution loop that all communicate with the same underlying I/O object.
* for example, a TCP socket can be made to accept many connections on the same endpoint from multiple threads.
* this can be problematic when it comes to **synchronization between multiple threads accessing the same shared resource**.
* Boost.Asio offers _strands_, which **serialize the execution of multiple callback handlers that are called from concurrent instances of the `run` function**.
  * a strand is like a funnel that forces multiple operations to occur in order.

* 