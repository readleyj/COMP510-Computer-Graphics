LDLIBS = -lglut -lGLEW -lGL -lGLU

CXXINCS = -I../../../include

INIT_SHADER = ../../../Common/InitShader.cpp

rubics_cube:
	g++ $(CXXINCS) $(INIT_SHADER) main.cpp $(LDLIBS) -o $@
	
clean:
	rm rubics_cube
