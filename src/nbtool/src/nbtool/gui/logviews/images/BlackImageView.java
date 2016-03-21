package nbtool.gui.logviews.images;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.geom.Ellipse2D;
import java.awt.Color;
import java.awt.event.ComponentAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;
import javax.swing.JButton;

import java.io.File;
import java.io.IOException;
import javax.swing.JFileChooser;

import java.util.ArrayList;

import nbtool.data.Log;
import nbtool.data.SExpr;
import nbtool.io.FileIO;
import nbtool.gui.logviews.misc.ViewParent;
import nbtool.util.Utility;

public class BlackImageView extends ViewParent implements MouseListener, MouseMotionListener, ActionListener {

	private int logHeight = 480;

	BufferedImage logImg;
	private JButton save;
	private JButton undo;
	private JButton reset;
	private String label = null;

	private Ellipse2D current_ball = null;
	private ArrayList<Ellipse2D> ball_list = null;

	private boolean drawing_circle = false;

	public void paintComponent(Graphics g) {
		super.paintComponent(g);
		Graphics2D g2d = (Graphics2D)g;

		if (logImg != null) {
			g.drawImage(logImg,0,0,null);
		}
		if (label != null) {
			g.drawString(label, 280, logHeight + 35);
		}

		if(ball_list != null) { //display all existing balls in the log
			for(Ellipse2D ball : ball_list) {
				g2d.setColor(Color.red);
				g2d.draw(ball);
			}
		}
	}

	public void setLog(Log newlog) {
		this.logImg = Utility.biFromLog(newlog);
		repaint();
	}

	public BlackImageView() {
		super();
		setLayout(null);

		ball_list = new ArrayList<Ellipse2D>();

		this.addMouseMotionListener(this);
		this.addMouseListener(this);

		save = new JButton("save");
		save.addActionListener(this);
		save.setPreferredSize(new Dimension(70,25));
		save.setBounds(10, logHeight + 20, 70, 25);
		this.add(save);

		undo = new JButton("undo");
		undo.addActionListener(this);
		undo.setPreferredSize(new Dimension(70,25));
		undo.setBounds(100, logHeight + 20, 70, 25);
		this.add(undo);

		reset = new JButton("reset");
		reset.addActionListener(this);
		reset.setPreferredSize(new Dimension(70,25));
		reset.setBounds(190, logHeight + 20, 70, 25);
		this.add(reset);
	}

	@Override
	public void mouseDragged(MouseEvent e) {
		if(drawing_circle) {
			double ball_width = e.getX() - current_ball.getX();
			double ball_height = e.getY() - current_ball.getY();
			current_ball.setFrame(current_ball.getX(), current_ball.getY(), ball_width, ball_height);
			repaint();
		}
	}

	@Override 
	public void mouseMoved(MouseEvent e) {
		if (logImg == null || log == null) {
			return;
		}

		int col = e.getX();
		int row = e.getY();

		if(col < 0 || row < 0 || col >= logImg.getWidth() || row >= logImg.getHeight()) {
			return;
		}

		label = String.format("Current Position: (%d,%d)", col, row);
		repaint();
	}

	public void mousePressed(MouseEvent e) {
		if(!drawing_circle) {

			int col = e.getX();
			int row = e.getY();

			if(col < 0 || row < 0 || col >= logImg.getWidth() || row >= logImg.getHeight()) {
				return;
			}
			current_ball = new Ellipse2D.Double(e.getX(),e.getY(),0,0);
			drawing_circle = true;
			repaint();
		}
	}

	public void mouseReleased(MouseEvent e) {
		if(drawing_circle) {
			double ball_width = e.getX() - current_ball.getX();
			double ball_height = e.getY() - current_ball.getY();
			current_ball = new Ellipse2D.Double(current_ball.getX(), current_ball.getY(), ball_width, ball_height);
			ball_list.add(current_ball);
			current_ball = null;
			drawing_circle = false;
		}
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if(e.getSource() == save && log != null) {
			for(Ellipse2D ball : ball_list) {
				SExpr s = ball_info_to_sexpr(ball);
				if(s == null || s.count() < 1) {
					System.out.println("error");
				} else {
					//log.append(s);
				}
			}
			int rVal = FileIO.fileChooser.showSaveDialog(this);
			if(rVal == JFileChooser.APPROVE_OPTION) {
				File f = FileIO.fileChooser.getSelectedFile();
				if(f.isDirectory()) {
					return;
				}
				String aPath = f.getAbsolutePath();
				if(!aPath.endsWith(".nblog")) {
					aPath = aPath + ".nblog";
				}

				try  {
					FileIO.writeLogToPath(log,aPath);
				} catch (IOException ioe) {
					ioe.printStackTrace();
				}
			}
		} else if(e.getSource() == undo) {
			ball_list.remove(ball_list.size()-1);
			repaint();
		} else {
			ball_list.clear();
			repaint();
		}
	}

	private SExpr ball_info_to_sexpr(Ellipse2D ball) {
		SExpr s;
		s = SExpr.list(
			SExpr.newKeyValue("x",ball.getX()),
			SExpr.newKeyValue("y",ball.getY()),
			SExpr.newKeyValue("width",ball.getWidth()),
			SExpr.newKeyValue("height",ball.getHeight())
			);
		return s;
	}

	private void save_ball_info_to_log() {
		for(Ellipse2D ball : ball_list) {
			
		}
	}

	public void mouseEntered(MouseEvent e){}
	public void mouseExited (MouseEvent e){}
	public void mouseClicked(MouseEvent e){}
}