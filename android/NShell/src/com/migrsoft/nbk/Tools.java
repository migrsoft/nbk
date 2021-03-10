package com.migrsoft.nbk;

import java.io.File;
import java.io.FileOutputStream;

import android.graphics.Bitmap;
import android.util.Log;

public class Tools {
	
	private static final String TAG = "NbkTools";
	
	public static void saveBitmapToFile(Bitmap bitmap, String path) {
		
		Log.i(TAG, "save bitmap to " + path);
		
		File f = new File(path);
		if (f.exists()) f.delete();
		
		try {
			FileOutputStream out = new FileOutputStream(f);
			bitmap.compress(Bitmap.CompressFormat.PNG, 90, out);
			out.flush();
			out.close();
		}
		catch (Exception e) {
		}
	}
}
