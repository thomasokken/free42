CABWIZ = "/cygdrive/c/Program Files/Windows CE Tools/wce420/Pocket PC 2003/Tools/CabWiz.exe"

default: skins.cpp keymap.cpp

skin2cpp: skin2cpp.cpp
	$(CXX) -DWINDOWS -o skin2cpp skin2cpp.cpp

skins.cpp: skin2cpp skin2cpp.conf
	./skin2cpp

keymap2cpp: keymap2cpp.cpp
	$(CXX) -DWINDOWS -o keymap2cpp keymap2cpp.cpp

keymap.cpp: keymap2cpp keymap.txt
	./keymap2cpp

cabs: FORCE
	$(CABWIZ) Free42Decimal.inf /cpu PPC2003_ARM PPC2003_x86
	$(CABWIZ) Free42Binary.inf /cpu PPC2003_ARM PPC2003_x86

package: ARMV4RelDecimal/Free42Decimal.exe ARMV4RelBinary/Free42Binary.exe
	$(CABWIZ) Free42Decimal.inf /cpu PPC2003_ARM
	$(CABWIZ) Free42Binary.inf /cpu PPC2003_ARM
	mkdir Free42PocketPC
	cp README.txt Free42PocketPC
	cp Free42Decimal.PPC2003_ARM.CAB Free42PocketPC/Free42Decimal.cab
	cp Free42Binary.PPC2003_ARM.CAB Free42PocketPC/Free42Binary.cab
	rm -f Free42PocketPC.zip
	zip -r Free42PocketPC.zip Free42PocketPC
	rm -rf Free42PocketPC

clean: FORCE
	rm -rf skin2cpp.exe skins.cpp keymap2cpp.exe keymap.cpp \
		Free42Decimal.*.CAB Free42Decimal.*.DAT \
		Free42Binary.*.CAB Free42Binary.*.DAT
		Free42PocketPC Free42PocketPC.zip

FORCE:
