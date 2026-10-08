# Strict interface binding

An explicit `eMule/NetworkInterface` selection is mandatory: startup fails if
that interface is missing, down, or cannot be bound. TCP clients, TCP listeners,
and UDP sockets refuse to continue after a native bind failure. With no interface
selected, networking follows the system routing configuration.

The core checks the selected interface every second. Loss of its usable addresses
or a change in its index triggers a clean shutdown. Restart after restoring the
interface. This is application-level protection; the interval before shutdown and
OS-specific routing behavior mean it does not replace a firewall kill switch.

HTTP requests also refuse a failed bind. A backend without an interface-binding
callback (including the Windows WinHTTP backend) blocks HTTP updates when an
interface is selected. Local EC listeners retain their explicit interface override.

Validation: `amuled` and `LibSocketTransportTest`, including TCP connect/listen
rejection for a nonexistent interface. Runtime interface removal and Windows/macOS
native binding still require platform integration tests.
