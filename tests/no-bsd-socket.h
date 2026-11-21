/* Hide the two socket names Apple headers omit under
 * _POSIX_C_SOURCE=200809L. A use of either is then an undeclared
 * identifier on any host. */

#include <netinet/in.h>
#include <sys/socket.h>

#undef INADDR_LOOPBACK
#undef MSG_DONTWAIT
#define INADDR_LOOPBACK inaddr_loopback_is_not_posix
#define MSG_DONTWAIT msg_dontwait_is_not_posix
#define POSIX_NET_FORCE_FALLBACK 1
