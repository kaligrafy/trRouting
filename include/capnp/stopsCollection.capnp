@0xe4f4eb0e4c776b38;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("stopsCollection");

struct StopsCollection {
  stops @0 :List(Stop);
}

struct Stop {
  id @0 :Int64;
  uuid @1 :Text;
  code @2 :Text;
  name @3 :Text;
  latitude @4 :Float32;
  longitude @5 :Float32;
  stationId @6 :Int32;
}

