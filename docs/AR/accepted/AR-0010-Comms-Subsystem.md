---
GitHub-Issue: #74
---

# AR-0010 — Comms Subsystem (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

## Context

IrisOS requires a unified object model for communication across diverse physical links and
protocols: TCP/UDP, serial, USB, FireWire, HDMI control channels, and more. These vary in
transport semantics, but can be unified through a small set of abstractions.

## Recommendation

Define **Erector::Comms** as a top-level subsystem for communication objects and protocol stacks.
Comms provides addressable, waitable communication objects while remaining separate from CEO
(scheduling) and Erector::Exec (synchronization primitives).

### Architectural Split

1) **Link layer objects (the "wire" or bus)**
   - SerialPort (RS-232/422/485 variants)
   - UsbEndpoint / UsbInterface
   - FireWireNode / IsochronousChannel
   - EthernetInterface / WifiInterface
   - BluetoothLink (optional)

2) **Transport/session objects (conversation semantics)**
   - TcpSocket
   - UdpSocket
   - TlsSession
   - SshSession
   - WebSocket (later)

3) **Protocol objects (framing + parsing)**
   - IPv4, IPv6
   - Arp, Icmp
   - DhcpClient
   - DnsResolver
   - SerialFramer (SLIP/COBS/HDLC-style framing)

4) **Addressing & identity**
   - IpAddress, MacAddress, Port
   - UsbDeviceId (VID/PID)
   - SerialParams (baud/parity/stop bits)
   - EndpointAddress

### Universal Primitive: Channel

Define base abstractions to unify diverse links:

- `Comms::Channel` (common base)
- `Comms::ByteStream` (ordered bytes, e.g., TCP, serial)
- `Comms::DatagramPort` (packet boundaries, e.g., UDP)

### CEO Integration (One-Sentence Rule)

Comms objects expose async operations; CEO owns the reactors/pollers that make them wake up.

### Hardware Mapping

Drivers and hardware descriptors live in `Erector::Machine`, with Comms objects layered above:

- `Erector::Machine::NetworkAdapter` -> `Erector::Comms::EthernetInterface`
- `Erector::Machine::UartDevice` -> `Erector::Comms::SerialPort`
- `Erector::Machine::UsbController` -> `Erector::Comms::UsbInterface/Endpoint`

### HDMI Guidance

- Data-plane display: `Erector::Machine::DisplayOutput` / `DisplayMode`
- Control-plane messaging: `Erector::Comms::CecBus`
- Descriptor data: `Erector::Machine::EdidBlob`

## Notes

Comms uses Exec primitives (await, event, mailbox) but does not define them. It defines how data
moves into and out of the system while CEO ensures progress via I/O reactors.
