// Based on libssh2_channel_direct_tcpip_ex, see GitHub for full example
LIBSSH2_SESSION* session = ...;
// Connect to remote host, port
LIBSSH2_CHANNEL* channel = libssh2_channel_direct_tcpip(session, "localhost", 4000);
send_data_over_channel(channel, data);
libssh2_channel_free(channel);
libssh2_session_disconnect(session, "Bye");
