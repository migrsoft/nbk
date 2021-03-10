package com.wyl.nshell;

import android.content.Context;
import android.graphics.Color;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;

public class AddressBar extends LinearLayout {
	
	private static final String TAG = "NbkAddress";
	
	public interface IUserAction {
		public void OnUrl(String url);
	}
	
	EditText mEdit;
	Button mBtnGo;
	
	IUserAction	mUserAction;

	public AddressBar(Context context) {
		super(context);
		
		setOrientation(HORIZONTAL);
		setGravity(Gravity.CENTER_HORIZONTAL);
		setBackgroundColor(Color.WHITE);
		
		mEdit = new EditText(context);
		mEdit.setSingleLine();
		LinearLayout.LayoutParams editParams = new LinearLayout.LayoutParams(
				LinearLayout.LayoutParams.MATCH_PARENT,
				LinearLayout.LayoutParams.WRAP_CONTENT,
				1.0f);
		mEdit.setLayoutParams(editParams);
		addView(mEdit);
		
		mEdit.setImeOptions(EditorInfo.IME_ACTION_GO);
		mEdit.setOnEditorActionListener(new OnEditorActionListener() {

			@Override
			public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
				if (actionId == EditorInfo.IME_ACTION_GO) {
					openUrl();
					return true;
				}
				else
					return false;
			}
			
		});
		
		mEdit.setOnFocusChangeListener(new OnFocusChangeListener() {

			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				if (!hasFocus) {
					InputMethodManager imm = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
					imm.hideSoftInputFromWindow(mEdit.getWindowToken(), 0);
				}
			}
			
		});
		
		mBtnGo = new Button(context);
		mBtnGo.setText(R.string.go);
		LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(
				LinearLayout.LayoutParams.WRAP_CONTENT,
				LinearLayout.LayoutParams.WRAP_CONTENT);
		mBtnGo.setLayoutParams(btnParams);
		addView(mBtnGo);
		
		mBtnGo.setOnClickListener(new Button.OnClickListener() {

			@Override
			public void onClick(View v) {
				openUrl();
			}
		});
	}
	
	public void setUserActionListener(IUserAction listener) {
		mUserAction = listener;
	}

	private void openUrl() {
		String url = mEdit.getText().toString();
		if (url.length() > 0) {
			mUserAction.OnUrl(url);
		}
	}
}
