MCU_FLAGS := -mmcu=$(MCU) -DF_CPU=$(F_CPU)
SECTION_FLAGS := -ffunction-sections -fdata-sections
DEP_FLAGS := -MMD -MP

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
