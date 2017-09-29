all:demo_server demo_client

demo_server:server.c
	$(CC) $(LDFLAGS) $^ -o $@

demo_client:client.c
	$(CC) $(LDFLAGS) $^ -o $@

.PHONY:clean
clean:
	@-rm *.o demo_server demo_client
