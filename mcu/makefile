flash: 
	make -C "./mega2560/" flash

run: 
	make -C "./linux/" run

debug: 
	make -C "./linux/"
	cp ./linux/build/bin/linux_build debug

clean:
	make -C "./linux/" clean
	make -C "./mega2560/" clean
