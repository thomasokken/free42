default: skins.cpp keymap.cpp

skin2cpp: skin2cpp.cpp
	$(CXX) -DWINDOWS -o skin2cpp skin2cpp.cpp

skins.cpp: skin2cpp skin2cpp.conf
	./skin2cpp

keymap2cpp: keymap2cpp.cpp
	$(CXX) -DWINDOWS -o keymap2cpp keymap2cpp.cpp

keymap.cpp: keymap2cpp keymap.txt
	./keymap2cpp

clean: FORCE
	rm -rf skin2cpp.exe skins.cpp keymap2cpp.exe keymap.cpp

FORCE:
