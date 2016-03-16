package nbtool.gui.logviews.images;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.event.ComponentAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.image.BufferedImage;

import nbtool.data.Log;
import nbtool.gui.logviews.misc.ViewParent;
import nbtool.util.Utility;

public class BlackImageView extends ViewParent implements MouseMotionListener {
	BufferedImage logImg;
	private String label = null;

	public void paintComponent(Graphics g) {
		super.paintComponent(g);

		if (logImg != null) {
			g.drawImage(logImg,0,0,null);
		}
		if (label != null) {
			g.drawString(label, 10, logImg.getHeight() + 20);
		}
	}

	public void setLog(Log newlog) {
		this.logImg = Utility.biFromLog(newlog);
		repaint();
	}

	public BlackImageView() {
		super();
		setLayout(null);
		this.addMouseMotionListener(this);
	}

	@Override
	public void mouseDragged(MouseEvent e) {
		//get circle size here
	}

	@Override 
	public void mouseMoved(MouseEvent e) {

	}
}