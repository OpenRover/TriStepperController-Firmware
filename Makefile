TARGET?=main

build: compiledb
	@pio run -e $(TARGET)

upload: compiledb
	@pio run --target upload -e $(TARGET)

compiledb:
	@pio run -t compiledb -e $(TARGET)
	scripts/compiledb.py

monitor:
	@driver/monitor.py -v

emulator:
	@cd emulator && make emulator

clean:
	@rm -r *.o *.dSYM
	@pio run -t clean
	@cd emulator && make clean

.PHONY: build/* upload/* emulator compiledb
