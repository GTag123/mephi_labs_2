all: compile run

compile:
	cmake -S . -B cmake-build
	cd cmake-build && make

run: compile
	./cmake-build/download-file

clean:
	rm -rf cmake-build
