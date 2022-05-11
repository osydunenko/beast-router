# Beast Router

## Contents

- [Synopsis](#synopsis)
- [Dependencies](#dependencies)
- [Documentation](#documentation)
- [Building](#building)
- [Usage](#usage)
- [Limitations](#limitations)

## Synopsis {#synopsis}

This is yet another C++ microframework for running web services.
The framework is build on top of [Boost.Beast](https://github.com/boostorg/beast) and aimed the
simpflication of the routing creation for the incoming http requests.

The library is designed for:
- **Easy Routing:** Along with the chaining
- **Type Safe handlers:** HTTP/1
- **Header only library:** Uses C++17
- **Symmetry:** Role-agnostic, supports building clients (in progress), servers, or both

## Dependencies {#dependencies}
- Boost Libraries >= 1.74.0 as sys. dependency
	- Boost.System
	- Boost.Thread
	- Boost unit testing
	- Boost.Asio
	- Boost.Beast
- Doxygen >= 1.9.2

## Documentation {#documentation}

For the `api` doc generation you need to execute a target by using your generator passed to cmake.
The respective target is "excluded from all" (requires a standalone execution) which is called as `doc`
The target is activated via the `ROUTER_DOXYGEN` which is `OFF` by default.

So, see the following as a reference of the docu generation:

```bash
cmake -DROUTER_DOXYGEN=ON ...
make doc
open doc/html/index.html
```

## Building {#building}
`beast-router` is a header-only. So, to use it just add the necessary `#include` line to your source code, such as the following:

```cpp
#include "beast_router.hpp"
```

Add the library as a submodule/Fetch content and then link to the respective target

```cmake
target_link_libraries(
	...
	PRIVATE
		...
		beast_router::beast_router
		...
)
```

## Usage {#usage}

The examples which you can build and run are in the `example` directory -- [Link](https://github.com/osydunenko/beast-router/tree/main/examples)

## Limitations {#limitations}
- WebSocket support
- TLS/Secure sockets
