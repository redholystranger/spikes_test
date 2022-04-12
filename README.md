Build:
  mkdir ./build

  cmake ..

  make


Run:

For FIX test:  ./latency_test <ip> <port> <account> <passwd> <SenderCompID> <TargetCompID>  > /tmp/results

For WS test:   ./ws_latency_test > /tmp/results


Get stat:

  ../scripts/get_stat.sh /tmp/results

  ../scripts/draw.py /tmp/results

  display /tmp/results.png
 
