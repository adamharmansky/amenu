all:
	cc menu.c draw.c -lX11 -lXft -lpthread -g -o menu

run: all
	./menu

debug: all
	gdb -tui ./menu

clean:
	rm -rf menu

install: all
	mkdir -p /usr/share/amenu
	echo "copying icons..."
	cp -r icons /usr/share/amenu/
	echo "done!"
	cp menu /usr/bin/amenu
	chmod +x /usr/bin/amenu
