package com.wyl.nshell;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.EditText;

public class OpenUrlDialog extends DialogFragment {
	
	public interface OpenUrlDialogListener {
		public void onOpenUrl(String url);
	}
	
	private OpenUrlDialogListener mListener;
	
	public void setListener(OpenUrlDialogListener listener) {
		mListener = listener;
	}
	
	private EditText mEditUrl;

	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState) {
		AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
		LayoutInflater inflater = getActivity().getLayoutInflater();

		View dlgContent = inflater.inflate(R.layout.dialog_url, null);
		mEditUrl = (EditText) dlgContent.findViewById(R.id.edit_url);
		
		builder.setTitle(R.string.dlg_openurl_title);
		builder.setView(dlgContent);
		
		builder.setPositiveButton(R.string.open, new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int id) {
				// 打开 URL
				String url = mEditUrl.getText().toString();
				if (TextUtils.isEmpty(url))
					return;
				if (mListener != null)
					mListener.onOpenUrl(url);
				OpenUrlDialog.this.getDialog().dismiss();
			}
		});
		
		builder.setNegativeButton(R.string.cancel, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
				OpenUrlDialog.this.getDialog().cancel();
			}
		});
		
		return builder.create();
	}

}
