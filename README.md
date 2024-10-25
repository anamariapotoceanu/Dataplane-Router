## Task 1 - Dataplane Router

- **Packet Type Check**: The type of the received packet is verified. If it is determined to be of type IPv4, the corresponding function (`function_ipv4`) is called.

- **Processing IPv4 Packet**:
  - Within this function, I first checked whether the packet's destination is the router and if it receives a message intended for it (type 8 for ICMP). In this case, the router does not forward the packet but attempts to process it (`verify_destination_icmp`).
  - The next step was to verify the checksums. If the checksum differs from the one received in the header, it means the packet has been corrupted and will not be forwarded.
  - For TTL, I checked if it is less than or equal to 0, which means the packet cannot be forwarded. Otherwise, I decremented the TTL.
  - I looked up the next hop address in the routing table. If no route is found, an ICMP type 3 message is sent. I used a binary search method to find the route; first, I sorted the routing table based on the prefix and mask.
  - If we successfully find the next hop address, we look up the corresponding MAC address in the ARP table to send the packet.
  - Before sending the packet, we ensure that the checksum is recalculated, set the source and destination MAC addresses, and send the packet through the interface of the previously found next hop.

- **Sending ICMP Packets**: I created a function (`function_icmp`) that, depending on the type of the message, will set the appropriate type and specific code:
  - Type 3 corresponds to the case when the destination cannot be found.
  - Type 0 is for the message received by the router that needs to respond to itself.
  - Type 11 is for TTL.

  For each ICMP packet, the type and code will be set in the ICMP header, and the protocol field in the IP header is dedicated to ICMP messages.

- **Header Construction**: To send packets, it is necessary to construct the three headers. The destination will be the new source, and the old source will become the new destination. I used copies of the `ether_header_copy` and `ip_header_copy` headers to retain these aspects and recalculated the checksums for the two headers.
  - The length of the packet to be sent is the length of the original packet plus the size of the `struct icmphdr` structure.
  - Finally, I created a new packet with the content, interface, and required length for sending the packet.
