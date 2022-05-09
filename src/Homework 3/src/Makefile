LDLIBS = -lglut -lGLEW -lGL -lGLU

CXXINCS = -I../../../include

INIT_SHADER = ../../../Common/InitShader.cpp

bouncing_ball:
	g++ $(CXXINCS) $(INIT_SHADER) main.cpp $(LDLIBS) -o $@
	
clean:
	rm bouncing_ball
