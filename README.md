
## ABOUT

This is a new verion of the flux tool which only focuses on the event-based run- time.  The original is found at:

http://flux.cs.umass.edu/

## IMPROVEMENTS ON ORIGINAL

1. State machine is not hardcoded as a switch statement (not very scalable)
2. Live loading of the flow graph from an XML file is possible
3. Nicer memory management
4. Minimal code in the generated source (more reliance on a run-time library)
5. If there is an empty event queue and no work to do, then there is blocking. 
   so CPU goes to 0 until one outstanding blocking call returns (efficiency)
6. Limits on thread growth
7. Guards in place of atomic constraints.  These are more "map-like" and 
   are useful for keeping shared application state.  Different access
   semantics (exclusive, read-write, pool) are available.
8. Module system for code re-use
9. Detached nodes which are treated as if they were shimmed system calls
10. Plugin system which allows for dependencies growing away from a minimal
   "kernel" program
11. New run-time built upon lock-free data structures (32-bit Intel x86)
12. Dynamic reloading of the XML flow supported
13. Multiple successor events available via two different mechanisms


## REQUIREMENTS

Either Solaris 10 or Linux supported

OCaml (used version 3.09.1 or later)

g++   (used version 4.0.3 or later)

stl C++ libraries

expat


## BUILDING


   make all

## EXAMPLES

The default build builds these.

The webserver is based on the original stock flux example.
You run it with
	./builds/*/run-webserver.sh 8080 /tmp
(for instance)


