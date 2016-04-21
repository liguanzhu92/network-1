## README
# Author
Guanzhu Li, Jiabei Xu
# Network Topology


           1Mb/s, 10ms       1Mb/s, 10ms       1Mb/s, 10ms
       A-----------------B-----------------C-----------------D


 - Tracing of queues and packet receptions to file 
   "tcp-large-transfer.tr"
 - pcap traces also generated in the following files
   "tcp-large-transfer-$n-$i.pcap" where n and i represent node and interface
 numbers respectively


# File Location:
	NS3_root/scratch/tcp_transfer.cc

# Run:
 	NS3_root/waf --run "tcp_transfer"

# Transimission Info:
	tcp-large-transfer.tr
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-0-0.pcap
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-1-0.pcap
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-1-1.pcap
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-2-1.pcap
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-3-0.pcap
	/usr/sbin/tcpdump -nn -tt -v -K -r  tcp-large-transfer-2-0.pcap


# Graph:
1. Plot the transmitted sequence number vs. time
  1. export the transimission Info at server
        ```
        /usr/sbin/tcpdump -nn -tt -r  tcp-large-transfer-0-0.pcap | awk _F'[:\t]*' "{if($8 == "seq" && $3== "10.1.1.1.49153")print($1," ",$9)}' > seq_num
        ```
  2. find the retransmit sequence number
    - compile the code
        ```
        javac FindDuplicates.java
        ```
    - then, put tranfer to the same folder of java class file and run the java program.
        ```
        java FindDuplicates > seq_num_retransmit
        ```
  3. Plot the Sequence VS time graph
        ``` 
        set term png
        set output "1.png"
        plot "seq_num" with linespoints, "seq_num_retransmit" with points
        ```

2. Plot cwnd vs. time
  1. save the log information
        ```
        ./waf --run "tcp-transfer -v" > log_info 2>&1
        ```
  2. save slow start window and time info
        ```
        cat log_info | grep Slow Starts | awk _F'[:\t]*' 'print($1,"",$13)}' > SlowCwndChange.cwnd
        ```
  3. Plot cwnd vs. time graph
        ```
        set term png
        set output "2.png"
        plot "CwndChange.cwnd" with linespoints, "SlowCwndChange.cwnd" with points
        ```
3. Plot TCPFixed and TCPNewReno
  1. comment out line 58 in tcp_transfer.cc
  2. copy "tcp-fixed.h" "tcp-fixed.cc" to ``NS3_root/src/internet/model``
  3. add `'model/tcp-fixed.h'` and `'model/tcp-fixed.cc'` to ``NS3_root/src/internet/model/wscript``
  4. re-build the file
        ```
        NS3_root/waf --run "tcp_transfer"
        ```
  5. export the transimission info at server
        ```
        /usr/sbin/tcpdump -nn -tt -r  tcp-large-transfer-0-0.pcap | awk _F'[:\t]*' "{if($8 == "seq" && $3 == "10.1.1.1.49153")print($1," ",$9)}' > fixed_seq_num
        ```
  6. Plot the TCPFixed VS TCPNewReno
        ```
        set term png
        set output "3.png"
        plot "seq_num" with linespoints, "fixed_seq_num" with linespoints
        ```
