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