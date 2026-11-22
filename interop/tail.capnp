@0xa4c0e11a77b09d31;

struct EchoResult {
  n @0 :Int32;
}

interface Echo @0xe401c0ffee000001 {
  echo @0 (n :Int32) -> EchoResult;
}

interface Hub @0xe401c0ffee000002 {
  relay @0 (cb :Echo, n :Int32) -> EchoResult;
}
