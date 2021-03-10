#include "script.h"
#include "js_keywords.h"

static int l_env_ref = 0;

static void env_init(void)
{
	if (l_env_ref == 0) {
		js_keywordsInit();
	}
	l_env_ref++;
}

static void env_release(void)
{
	l_env_ref--;
	if (l_env_ref == 0) {
		js_keywordsRelease();
	}
}

static void script_handleToken(JToken* token, void* user)
{
    JScript* script = (JScript*)user;
    JMessage msg;
    msg.type = JSMSG_TOKEN;
    msg.d.token = token;
    script_sendMsg(script, &msg);
}

JScript* script_create(void)
{
	JScript* s = (JScript*)NBK_malloc0(sizeof(JScript));
	s->scanner = scanner_create();
	s->scanner->script = s;
    s->scanner->handleToken = script_handleToken;
	env_init();
	return s;
}

void script_delete(JScript** script)
{
	JScript* s = *script;
	*script = N_NULL;
	scanner_delete(&s->scanner);
	NBK_free(s);
	env_release();
}

void script_setMsgListener(JScript* script, JMSG_LISTENER listener)
{
	script->msg_listener = listener;
}

void script_sendMsg(JScript* script, JMessage* msg)
{
	if (script->msg_listener)
		script->msg_listener(msg);
}

void script_execute(JScript* script, char* source, int length)
{
	scanner_write(script->scanner, source, length);
}
