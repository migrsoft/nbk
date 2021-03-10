package com.migrsoft.nbk;

import android.graphics.Canvas;

public interface INbkGlue {
	
	public int getScreenWidth();
	
	public int getScreenHeight();
	
	public Canvas getCanvas();
	
	public void sendNbkEventToCore(NbkEvent e);
	
	public void sendNbkEventToShell(NbkEvent e);

}
