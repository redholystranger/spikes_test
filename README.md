Build:
  mkdir ./build
  cmake ..
  make

Run:
  ./latency_test <ip> <port> <account> <passwd> <SenderCompID> <TargetCompID>  > /tmp/results

Get stat:
  ../scripts/get_stat.sh /tmp/results
  ../scripts/draw.py /tmp/results
  display /tmp/results.png
 
