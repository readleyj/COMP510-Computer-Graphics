LDLIBS = -lglut -lGLEW -lGL -lGLU

CXXINCS = -I../../../include

INIT_SHADER = ../../../Common/InitShader.cpp

bouncing_ball:
	g++ -$(CXXINCS) $(INIT_SHADER) sierpinski.cpp $(LDLIBS) -o $@
	
clean:
	rm sierpinkski
