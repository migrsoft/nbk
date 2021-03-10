#INC = -I/usr/include/SDL/
#LIBS = -L/usr/lib -lSDL -lSDL_ttf -lSDL_net -lSDL_image
#LDFLAGS =
#CC = gcc -g $(INC)
#BUILD = ../build/


PalmPDK = /opt/PalmPDK
LIBS = -lSDL -lSDL_ttf -lSDL_net -lSDL_image -lGLESv2 -lpdl -lz

SYSROOT = $(PalmPDK)/arm-gcc/sysroot
INCDIR = $(PalmPDK)/include
LIBDIR = $(PalmPDK)/device/lib
CFLAGS = -I$(INCDIR) -I$(INCDIR)/SDL --sysroot=$(SYSROOT)
LDFLAGS = -L$(LIBDIR) -Wl,--allow-shlib-undefined
DEVICEOPTS = -mcpu=arm1136jf-s -mfpu=vfp -mfloat-abi=softfp
CC = arm-none-linux-gnueabi-gcc $(DEVICEOPTS) $(CFLAGS) -O
BUILD = ../device/

#INCDIR	= ${PalmPDK}/include
#LIBDIR	= ${PalmPDK}/host/lib
#CFLAGS	= -I${INCDIR} -I${INCDIR}/SDL
#LDFLAGS	= -L${LIBDIR}
#CC		= gcc $(CFLAGS) -g
#BUILD	= ../host/

all: css dom editor loader render tools wbxml nbk
	$(CC) $(LDFLAGS) $(LIBS) $(BUILD)*.o -o $(BUILD)nbk

css:
	$(CC) -c ../stdc/css/color.c -o $(BUILD)color.o
	$(CC) -c ../stdc/css/css_helper.c -o $(BUILD)css_helper.o
	$(CC) -c ../stdc/css/css_prop.c -o $(BUILD)css_prop.o
	$(CC) -c ../stdc/css/cssSelector.c -o $(BUILD)cssSelector.o
	$(CC) -c ../stdc/css/css_value.c -o $(BUILD)css_value.o
	
dom:
	$(CC) -c ../stdc/dom/attr.c -o $(BUILD)attr.o
	$(CC) -c ../stdc/dom/char_ref.c -o $(BUILD)char_ref.o
	$(CC) -c ../stdc/dom/document.c -o $(BUILD)document.o
	$(CC) -c ../stdc/dom/event.c -o $(BUILD)event.o
	$(CC) -c ../stdc/dom/history.c -o $(BUILD)history.o
	$(CC) -c ../stdc/dom/incParser.c -o $(BUILD)incParser.o
	$(CC) -c ../stdc/dom/miParser.c -o $(BUILD)miParser.o
	$(CC) -c ../stdc/dom/node.c -o $(BUILD)node.o
	$(CC) -c ../stdc/dom/page.c -o $(BUILD)page.o
	$(CC) -c ../stdc/dom/view.c -o $(BUILD)view.o
	$(CC) -c ../stdc/dom/wbxmlDec.c -o $(BUILD)wbxmlDec.o
	$(CC) -c ../stdc/dom/xml_atts.c -o $(BUILD)xml_atts.o
	$(CC) -c ../stdc/dom/xml_helper.c -o $(BUILD)xml_helper.o
	$(CC) -c ../stdc/dom/xml_tags.c -o $(BUILD)xml_tags.o
	$(CC) -c ../stdc/dom/xml_tokenizer.c -o $(BUILD)xml_tokenizer.o
	$(CC) -c ../stdc/dom/xpath.c -o $(BUILD)xpath.o

editor:
	$(CC) -c ../stdc/editor/editBox.c -o $(BUILD)editBox.o
	$(CC) -c ../stdc/editor/textEditor.c -o $(BUILD)textEditor.o
	$(CC) -c ../stdc/editor/formData.c -o $(BUILD)formData.o
	$(CC) -c ../stdc/editor/textSel.c -o $(BUILD)textSel.o
	
loader:
	$(CC) -c ../stdc/loader/loader.c -o $(BUILD)loader.o
	$(CC) -c ../stdc/loader/ngzip.c -o $(BUILD)ngzip.o
	$(CC) -c ../stdc/loader/pkgParser.c -o $(BUILD)pkgParser.o
	$(CC) -c ../stdc/loader/upCmd.c -o $(BUILD)upCmd.o
	$(CC) -c ../stdc/loader/param.c -o $(BUILD)param.o
	$(CC) -c ../stdc/loader/url.c -o $(BUILD)url.o
	$(CC) -c ../stdc/loader/crypto.c -o $(BUILD)crypto.o
	$(CC) -c ../stdc/loader/idea.c -o $(BUILD)idea.o
	$(CC) -c ../stdc/loader/keygen.c -o $(BUILD)keygen.o

render:
	$(CC) -c ../stdc/render/imagePlayer.c -o $(BUILD)imagePlayer.o
	$(CC) -c ../stdc/render/layoutStat.c -o $(BUILD)layoutStat.o
	$(CC) -c ../stdc/render/renderNode.c -o $(BUILD)renderNode.o
	$(CC) -c ../stdc/render/renderBlank.c -o $(BUILD)renderBlank.o
	$(CC) -c ../stdc/render/renderInline.c -o $(BUILD)renderInline.o
	$(CC) -c ../stdc/render/renderBlock.c -o $(BUILD)renderBlock.o
	$(CC) -c ../stdc/render/renderInlineBlock.c -o $(BUILD)renderInlineBlock.o
	$(CC) -c ../stdc/render/renderBr.c -o $(BUILD)renderBr.o
	$(CC) -c ../stdc/render/renderHr.c -o $(BUILD)renderHr.o
	$(CC) -c ../stdc/render/renderA.c -o $(BUILD)renderA.o
	$(CC) -c ../stdc/render/renderImage.c -o $(BUILD)renderImage.o
	$(CC) -c ../stdc/render/renderInline.c -o $(BUILD)renderInline.o
	$(CC) -c ../stdc/render/renderInput.c -o $(BUILD)renderInput.o
	$(CC) -c ../stdc/render/renderTextarea.c -o $(BUILD)renderTextarea.o
	$(CC) -c ../stdc/render/renderSelect.c -o $(BUILD)renderSelect.o
	$(CC) -c ../stdc/render/renderObject.c -o $(BUILD)renderObject.o
	$(CC) -c ../stdc/render/renderTable.c -o $(BUILD)renderTable.o
	$(CC) -c ../stdc/render/renderTr.c -o $(BUILD)renderTr.o
	$(CC) -c ../stdc/render/renderTd.c -o $(BUILD)renderTd.o
	$(CC) -c ../stdc/render/renderText.c -o $(BUILD)renderText.o
	$(CC) -c ../stdc/render/renderTextPiece.c -o $(BUILD)renderTextPiece.o
	
tools:
	$(CC) -c ../stdc/tools/callback.c -o $(BUILD)callback.o
	$(CC) -c ../stdc/tools/dump.c -o $(BUILD)dump.o
	$(CC) -c ../stdc/tools/gdi.c -o $(BUILD)gdi.o
	$(CC) -c ../stdc/tools/memAlloc.c -o $(BUILD)memAlloc.o
	$(CC) -c ../stdc/tools/ptrArray.c -o $(BUILD)ptrArray.o
	$(CC) -c ../stdc/tools/slist.c -o $(BUILD)slist.o
	$(CC) -c ../stdc/tools/dlist.c -o $(BUILD)dlist.o
	$(CC) -c ../stdc/tools/strBuf.c -o $(BUILD)strBuf.o
	$(CC) -c ../stdc/tools/str.c -o $(BUILD)str.o
	$(CC) -c ../stdc/tools/strDict.c -o $(BUILD)strDict.o
	$(CC) -c ../stdc/tools/strList.c -o $(BUILD)strList.o
	$(CC) -c ../stdc/tools/strPool.c -o $(BUILD)strPool.o
	$(CC) -c ../stdc/tools/timer.c -o $(BUILD)timer.o
	$(CC) -c ../stdc/tools/tree.c -o $(BUILD)tree.o
	$(CC) -c ../stdc/tools/unicode.c -o $(BUILD)unicode.o

wbxml:
	$(CC) -c ../stdc/wbxml/wbxml_base64.c -o $(BUILD)wbxml_base64.o
	$(CC) -c ../stdc/wbxml/wbxml_buffers.c -o $(BUILD)wbxml_buffers.o
	$(CC) -c ../stdc/wbxml/wbxml_charset.c -o $(BUILD)wbxml_charset.o
	$(CC) -c ../stdc/wbxml/wbxml_elt.c -o $(BUILD)wbxml_elt.o
	$(CC) -c ../stdc/wbxml/wbxml_mem.c -o $(BUILD)wbxml_mem.o
	$(CC) -c ../stdc/wbxml/wbxml_parser.c -o $(BUILD)wbxml_parser.o
	$(CC) -c ../stdc/wbxml/wbxml_tables.c -o $(BUILD)wmxml_tables.o

nbk:
	$(CC) -c cache.c -o $(BUILD)cache.o
	$(CC) -c connfile.c -o $(BUILD)connfile.o
	$(CC) -c connhttp.c -o $(BUILD)connhttp.o
	$(CC) -c controls.c -o $(BUILD)controls.o
	$(CC) -c cookiemgr.c -o $(BUILD)cookiemgr.o
	$(CC) -c fileget.c -o $(BUILD)fileget.o
	$(CC) -c http.c -o $(BUILD)http.o
	$(CC) -c ini.c -o $(BUILD)ini.o
	$(CC) -c main.c -o $(BUILD)main.o
	$(CC) -c md5.c -o $(BUILD)md5.o
	$(CC) -c nbk.c -o $(BUILD)nbk.o
	$(CC) -c nbkgdi.c -o $(BUILD)nbkgdi.o
	$(CC) -c nbkpic.c -o $(BUILD)nbkpic.o
	$(CC) -c picmgr.c -o $(BUILD)picmgr.o
	$(CC) -c probe.c -o $(BUILD)probe.o
	$(CC) -c resmgr.c -o $(BUILD)resmgr.o
	$(CC) -c runtime.c -o $(BUILD)runtime.o
	$(CC) -c loginData.c -o $(BUILD)loginData.o
	$(CC) -c nbk_sdlExt.c -o $(BUILD)nbk_sdlExt.o
	$(CC) -c SDL_framerate.c -o $(BUILD)SDL_framerate.o
	$(CC) -c SDL_gfxBlitFunc.c -o $(BUILD)SDL_gfxBlitFunc.o
	$(CC) -c SDL_gfxPrimitives.c -o $(BUILD)SDL_gfxPrimitives.o
	$(CC) -c SDL_imageFilter.c -o $(BUILD)SDL_imageFilter.o
	$(CC) -c SDL_rotozoom.c -o $(BUILD)SDL_rotozoom.o
	$(CC) -c glutil.c -o $(BUILD)glutil.o

clean:
	rm $(BUILD)*.o
