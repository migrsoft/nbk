# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

MY_NBK_PLATFORM_PATH	:= ../../../java/NbkDll/
MY_NBK_PLATFORM_SRC		:= $(MY_NBK_PLATFORM_PATH)runtime.cpp
MY_NBK_PLATFORM_SRC		+= $(MY_NBK_PLATFORM_PATH)graphics.cpp
MY_NBK_PLATFORM_SRC		+= $(MY_NBK_PLATFORM_PATH)ImageMgr.cpp
MY_NBK_PLATFORM_SRC		+= $(MY_NBK_PLATFORM_PATH)LoaderMgr.cpp
MY_NBK_PLATFORM_SRC		+= $(MY_NBK_PLATFORM_PATH)NbkDll.cpp
MY_NBK_PLATFORM_SRC		+= $(MY_NBK_PLATFORM_PATH)resource.cpp

MY_NBK_STDC_PATH	:= ../../../stdc/
MY_NBK_STDC_SRC		:= $(MY_NBK_STDC_PATH)css/color.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)css/css_helper.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)css/css_prop.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)css/css_value.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)css/cssMgr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)css/cssSelector.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/attr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/char_ref.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/document.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/event.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/history.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/miParser.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/node.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/page.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/view.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/wbxmlDec.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/xml_atts.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/xml_helper.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/xml_tags.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/xml_tokenizer.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)dom/xpath.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)editor/editBox.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)editor/formData.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)editor/textEditor.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)editor/textSel.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)loader/loader.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)loader/md5.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)loader/upCmd.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)loader/url.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/imagePlayer.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/layoutStat.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderA.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderBlank.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderBlock.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderBr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderHr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderImage.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderInline.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderInlineBlock.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderInput.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderNode.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderObject.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderSelect.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderTable.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderTd.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderText.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderTextarea.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderTextPiece.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)render/renderTr.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/callback.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/dlist.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/dump.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/gdi.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/hashMap.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/memAlloc.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/memMgr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/probe.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/ptrArray.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/slist.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/smartPtr.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/str.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/strBuf.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/strDict.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/strList.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/strPool.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/timer.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)tools/unicode.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_base64.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_buffers.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_charset.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_elt.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_mem.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_parser.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)wbxml/wbxml_tables.c

MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)cpconv/conv.c
MY_NBK_STDC_SRC		+= $(MY_NBK_STDC_PATH)cpconv/uf_gbk.c

LOCAL_MODULE    := nbk
LOCAL_SRC_FILES := $(MY_NBK_PLATFORM_SRC)
LOCAL_SRC_FILES	+= $(MY_NBK_STDC_SRC)
LOCAL_LDLIBS    := -llog

include $(BUILD_SHARED_LIBRARY)
