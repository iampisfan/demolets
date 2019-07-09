OBJC := de_agent.c \
	single_ap.c \
	multi_ap.c \
        data_element.c \
        yang.c \
        utils.c \
        network.c \
        device.c \
        radio.c \
        bss.c \
        capabilities.c \
        sta.c \
        debug.c \
        nl80211.c \
        ubus.c \
        event.c \
        current_opclass.c \
	capable_opclass.c \
        opclass.c \
	association.c \
	disassociation.c \
	scan_result.c

de_agent:$(OBJC) libhttpd/src/libhttpd.a
	$(CC) -o $@ $^ -Ilibhttpd/src $(CFLAGS) $(LIBS)

libhttpd/src/libhttpd.a:
	$(MAKE) -C libhttpd all
