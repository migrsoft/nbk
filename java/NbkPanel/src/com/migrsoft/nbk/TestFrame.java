package com.migrsoft.nbk;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;

import javax.swing.JFrame;
import javax.swing.JTextField;
import javax.swing.border.Border;

public class TestFrame extends JFrame {

	/**
	 * 
	 */
	private static final long serialVersionUID = 5191427525740990133L;
	
	private JTextField mAddress;
	private NbkPanel mNbkPanel;
	
	private class AddressListener implements ActionListener {

		@Override
		public void actionPerformed(ActionEvent e) {
			if (e.getID() == ActionEvent.ACTION_PERFORMED) {
				loadUrl(mAddress.getText());
			}
		}
	}

	public TestFrame() {
		super("Test");
		setDefaultCloseOperation(EXIT_ON_CLOSE);
		
		mAddress = new JTextField();
		mAddress.addActionListener(new AddressListener());
		
		mNbkPanel = new NbkPanel();
		mNbkPanel.setPreferredSize(new Dimension(330, 490));
		
		Container contentPane = getContentPane();
		contentPane.add(mAddress, BorderLayout.NORTH);
		contentPane.add(mNbkPanel, BorderLayout.CENTER);

		this.addWindowListener(new WindowListener() {

			@Override
			public void windowActivated(WindowEvent e) {
			}

			@Override
			public void windowClosed(WindowEvent e) {
			}

			@Override
			public void windowClosing(WindowEvent e) {
				mNbkPanel.abortCore();
			}

			@Override
			public void windowDeactivated(WindowEvent e) {
			}

			@Override
			public void windowDeiconified(WindowEvent e) {
			}

			@Override
			public void windowIconified(WindowEvent e) {
			}

			@Override
			public void windowOpened(WindowEvent e) {
			}
		});
		
		pack();
		setResizable(false);
		setVisible(true);
	}
	
	private void loadUrl(String url) {
		if (url != null && url.length() > 0) {
			if (url.contains("://")) {
				mNbkPanel.loadUrl(url);
			}
			else if (url.contains(".")) {
				mNbkPanel.loadUrl("http://" + url);
			}
		}
	}

	private static TestFrame sInstance = null;
	
	public static void main(String[] args) {
		Runnable r = new Runnable() {

			@Override
			public void run() {
				sInstance = new TestFrame();
			}
		};
		
		EventQueue.invokeLater(r);
	}
}
