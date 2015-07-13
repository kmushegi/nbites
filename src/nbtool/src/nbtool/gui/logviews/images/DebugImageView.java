package nbtool.gui.logviews.images;

import java.awt.Graphics;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Shape;
import java.awt.geom.AffineTransform;
import java.awt.geom.Ellipse2D;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemListener;
import java.awt.event.ItemEvent;
import java.awt.event.ActionEvent;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.util.Vector;
import java.util.ArrayList;
import java.util.List;
import javax.swing.JComboBox;
import javax.swing.JCheckBox;
import javax.swing.JPanel;
import java.awt.GridLayout;

import nbtool.util.Logger;
import nbtool.data.Log;
import nbtool.data.SExpr;
import nbtool.gui.logviews.misc.ViewParent;
import nbtool.images.DebugImage;
import nbtool.images.Y8image;
import nbtool.images.EdgeImage;
import nbtool.io.CommonIO.IOFirstResponder;
import nbtool.io.CommonIO.IOInstance;
import nbtool.io.CrossIO;
import nbtool.io.CrossIO.CrossFunc;
import nbtool.io.CrossIO.CrossInstance;
import nbtool.io.CrossIO.CrossCall;
import nbtool.util.Utility;

public class DebugImageView extends ViewParent
	implements IOFirstResponder, ActionListener {

	// Values according to nbcross/vision_defs.cpp - must be kept in sync
	static final int YIMAGE = 0;
	static final int WHITE_IMAGE = 1;
	static final int GREEN_IMAGE = 2;
	static final int ORANGE_IMAGE = 3;
	static final int SEGMENTED = 4;
	static final int EDGE_IMAGE = 5;
	static final int LINE_IMAGE = 6;
	static final int BALL_IMAGE = 7;
	static final int CENTER_CIRCLE = 8;
	static final int DRAWING = 9;
	static final int ORIGINAL = 10;

	static final int DEFAULT_WIDTH = 320;
	static final int DEFAULT_HEIGHT = 240;

	static final int BUFFER = 5;
	static final int STARTSIZE = 3;
	static final int FIELDW = 640;
	static final int FIELDH = 554;

	// Images that we can view in this view using the combo box
	String[] imageViews = { "Original", "Green", "Orange", "White", "Edge" };
	JComboBox viewList;
	JPanel checkBoxPanel;
	JCheckBox showCameraHorizon;
	JCheckBox showFieldHorizon;
	JCheckBox debugHorizon;
	JCheckBox debugFieldEdge;
	JCheckBox debugBall;
	JCheckBox showFieldLines;
	CheckBoxListener checkListener = null;

	boolean displayFieldLines;
	boolean drawAllBalls;

	static final int NUMBER_OF_PARAMS = 5; // update as new params are added
	static int displayParams[] = new int[NUMBER_OF_PARAMS];

	// Dimensions of the image that we are working with
    int width;
    int height;

	// Dimensions as we want to display them
    int displayw;
    int displayh;

	Vector<Double> lines;
	Vector<Double> ccPoints;

    BufferedImage originalImage;            // what the robot saw
    DebugImage debugImage;                  // drawing overlay
	BufferedImage debugImageDisplay;        // overlay + original
	BufferedImage displayImages[] = new BufferedImage[ORIGINAL+1]; // our images

	Log currentLog;
	Log balls;

	static int currentBottom;  // track current selection
	static boolean firstLoad = true;
	boolean newLog = true;

	boolean parametersNeedSetting = false;

    public DebugImageView() {
        super();
        setLayout(null);
		// set up combo box to select views
		viewList = new JComboBox(imageViews);
		viewList.setSelectedIndex(0);
		viewList.addActionListener(this);

		// set up check boxes
		checkListener = new CheckBoxListener();
		showCameraHorizon = new JCheckBox("Show camera horizon");
		showFieldHorizon = new JCheckBox("Show field convex hull");
		debugHorizon = new JCheckBox("Debug Field Horizon");
		debugFieldEdge = new JCheckBox("Debug Field Edge");
		debugBall = new JCheckBox("Debug Ball");
		showFieldLines = new JCheckBox("Hide Field Lines");
		displayFieldLines = true;
		drawAllBalls = false;

		// add their listeners
		showCameraHorizon.addItemListener(checkListener);
		showFieldHorizon.addItemListener(checkListener);
		debugHorizon.addItemListener(checkListener);
		debugFieldEdge.addItemListener(checkListener);
		debugBall.addItemListener(checkListener);
		showFieldLines.addItemListener(checkListener);

		// put them into one panel
		checkBoxPanel = new JPanel();
		checkBoxPanel.setLayout(new GridLayout(0, 1)); // 0 rows, 1 column
		checkBoxPanel.add(showCameraHorizon);
		checkBoxPanel.add(showFieldHorizon);
		checkBoxPanel.add(debugHorizon);
		checkBoxPanel.add(debugFieldEdge);
		checkBoxPanel.add(debugBall);
		checkBoxPanel.add(showFieldLines);

		add(checkBoxPanel);
		add(viewList);

        this.addMouseListener(new DistanceGetter());

		// default all checkboxes to false
		showCameraHorizon.setSelected(false);
		showFieldHorizon.setSelected(false);
		debugHorizon.setSelected(false);
		debugFieldEdge.setSelected(false);
		debugBall.setSelected(false);
		showFieldLines.setSelected(false);

		// for now do not bother trying to save params across instances
		for (int i = 0; i < NUMBER_OF_PARAMS; i++) {
			displayParams[i] = 0;
		}
		// default image to display - save across instances
		if (firstLoad) {
			for (int i = 0; i < NUMBER_OF_PARAMS; i++) {
				displayParams[i] = 0;
			}

			firstLoad = false;
			currentBottom = ORIGINAL;
		} else {
			System.out.println("Reloading");
			// Ideally we'd do our debug drawing right away,
			// but this isn't easily possible until we shift
			// the tool to doing a single VisionModule instance
			/*for (int i = 0; i < NUMBER_OF_PARAMS; i++) {
				if (displayParams[i] != 0) {
					parametersNeedSetting = true;
					break;
				}
				}*/
		}
    }

    @Override
    public void setLog(Log newlog) {
        CrossInstance ci = CrossIO.instanceByIndex(0);
        if (ci == null)
            return;
        CrossFunc func = ci.functionWithName("Vision");
        assert(func != null);

        CrossCall cc = new CrossCall(this, func, newlog);

        assert(ci.tryAddCall(cc));

        Vector<SExpr> vec = newlog.tree().recursiveFind("width");
        if (vec.size() > 0) {
            SExpr w = vec.get(vec.size()-1);
            width =  w.get(1).valueAsInt() / 2;
        } else {
            System.out.printf("COULD NOT READ WIDTH FROM LOG DESC\n");
            width = DEFAULT_WIDTH;
        }

        vec = newlog.tree().recursiveFind("height");
        if (vec.size() > 0) {
            SExpr h = vec.get(vec.size()-1);
            height = h.get(1).valueAsInt() / 2;
        } else {
            System.out.printf("COULD NOT READ HEIGHT FROM LOG DESC\n");
            height = DEFAULT_HEIGHT;
        }

        displayw = width*2;
        displayh = height*2;

        displayImages[ORIGINAL] = Utility.biFromLog(newlog);
		currentLog = newlog;
    }

    /* Our parameters have been adjusted. Get their values, make an expression
	 * and ship it off to Vision.
    */
    public void adjustParams() {

        // Don't make an extra initial call
        if (newLog) {
			System.out.println("Skipping parameter adjustments");
            return;
		}
        //zeroParam();

        SExpr newParams = SExpr.newList(SExpr.newKeyValue("CameraHorizon", displayParams[0]),
										SExpr.newKeyValue("FieldHorizon", displayParams[1]),
										SExpr.newKeyValue("DebugHorizon", displayParams[2]),
										SExpr.newKeyValue("DebugField", displayParams[3]),
										SExpr.newKeyValue("DebugBall", displayParams[4]));

        // Look for existing Params atom in current this.log description
        SExpr oldParams = currentLog.tree().find("DebugDrawing");

        // Add params or replace params
        if (oldParams.exists()) {
            oldParams.setList( SExpr.atom("DebugDrawing"), newParams);
        } else {
            this.log.tree().append(SExpr.pair("DebugDrawing", newParams));
        }

        rerunLog();
		repaint();
    }


    public void paintComponent(Graphics g) {
		final int BOX_HEIGHT = 25;
        if (debugImage != null) {
            g.drawImage(debugImageDisplay, 0, 0, displayw, displayh, null);
			drawLines(g);
			drawBlobs(g);
			g.drawImage(displayImages[currentBottom], 0, displayh + 5, displayw,
						displayh, null);
			viewList.setBounds(0, displayh * 2 + 10, displayw / 2, BOX_HEIGHT);
			// TODO: figure out how to make this consistently display
			// The problem has to do with repaint and the fact that Java
			// treats it as a low priority request. Sometimes it will just
			// take its sweet time because it doesn't think anything has changed
			checkBoxPanel.setBounds(displayw+10, 0, displayw, displayh);
			checkBoxPanel.show();
        }
    }

	/* Taken from LineView.java
	 */
	public void drawLines(Graphics g) {
		// This code stolen from LineView.java
		// TODO: obviously this should be moved into its own function
		if (displayFieldLines) {
			for (int i = 0; i < lines.size(); i += 10) {
				double icR = lines.get(i);
				double icT = lines.get(i + 1);
				double icEP0 = lines.get(i + 2);
				double icEP1 = lines.get(i + 3);
				double houghIndex = lines.get(i + 4);
				double fieldIndex = lines.get(i + 5);
				double fcR = lines.get(i + 6);
				double fcT = lines.get(i + 7);
				double fcEP0 = lines.get(i + 8);
				double fcEP1 = lines.get(i + 9);

				// Draw it in image coordinates
				if (fieldIndex == -1)
					g.setColor(Color.red);
				else
					g.setColor(Color.blue);

				double x0 = 2*icR * Math.cos(icT) + displayImages[ORIGINAL].getWidth() / 2;
				double y0 = -2*icR * Math.sin(icT) + displayImages[ORIGINAL].getHeight() / 2;
				int x1 = (int) Math.round(x0 + 2*icEP0 * Math.sin(icT));
				int y1 = (int) Math.round(y0 + 2*icEP0 * Math.cos(icT));
				int x2 = (int) Math.round(x0 + 2*icEP1 * Math.sin(icT));
				int y2 = (int) Math.round(y0 + 2*icEP1 * Math.cos(icT));

				g.drawLine(x1, y1, x2, y2);

				// Image view line labels
				double xstring = (x1 + x2) / 2;
				double ystring = (y1 + y2) / 2;

				double scale = 0;
				if (icR > 0)
					scale = 10;
				else
					scale = 3;
				xstring += scale*Math.cos(icT);
				ystring += scale*Math.sin(icT);

				g.drawString(Integer.toString((int) houghIndex) + "/" +
							 Integer.toString((int) fieldIndex),
							 (int) xstring,
							 (int) ystring);
			}
		}
	}
	/* Taken directly from BallView.java (where it was undocumented). Draws blobs
	 * related to the ball.
	 */
    public void drawBlobs(Graphics g)
    {
		// if we don't have an orange image we're in trouble
        if (displayImages[ORANGE_IMAGE] == null) {
			System.out.println("No orange image");
			return;
		}
        //Graphics2D graph = orange.createGraphics();
		Graphics2D graph = (Graphics2D)g;
        graph.setColor(Color.RED);
        String b = "blob";

		// loop through all of the balls we find in the tree
		for (int i=0; ; i++)
			{
				SExpr tree = balls.tree();
				SExpr bl = tree.find(b+i);
				if (!bl.exists()) {
					break;
				}
				SExpr blob = bl.get(1);
				if (drawAllBalls) {
					drawBlob(graph, blob);
				}
			}

        graph.setColor(Color.GREEN);

        b = "ball";

        for(int i=0; ;i++)
        {
            SExpr tree = balls.tree();
            SExpr ball = tree.find(b+i);
            if (!ball.exists()){
                break;
            }
            SExpr blob = ball.get(1).find("blob").get(1);
            double diam = ball.get(1). find("expectedDiam").get(1).valueAsDouble();
            SExpr loc = blob.find("center").get(1);

            int x = (int) Math.round(loc.get(0).valueAsDouble());
            int y = (int) Math.round(loc.get(1).valueAsDouble());
            graph.draw(new Ellipse2D.Double((x - diam/2) * 2, (y - diam/2)*2, diam*2, diam*2));
        }
    }

    private void drawBlob(Graphics2D g, SExpr blob)
    {
        SExpr loc = blob.find("center").get(1);

        int x = (int) Math.round(loc.get(0).valueAsDouble());
        int y = (int) Math.round(loc.get(1).valueAsDouble());

        double len1 = blob.find("len1").get(1).valueAsDouble();
        double len2 = blob.find("len2").get(1).valueAsDouble();
        double ang1 = blob.find("ang1").get(1).valueAsDouble();
        double ang2 = blob.find("ang2").get(1).valueAsDouble();

        int firstXOff = (int)Math.round(len1 * Math.cos(ang1));
        int firstYOff = (int)Math.round(len1 * Math.sin(ang1));
        int secondXOff = (int)Math.round(len2 * Math.cos(ang2));
        int secondYOff = (int)Math.round(len2 * Math.sin(ang2));

        g.drawLine((x - firstXOff)*2, (y - firstYOff)*2,
				   (x + firstXOff)*2, (y + firstYOff)*2);
        g.drawLine((x - secondXOff)*2, (y - secondYOff)*2,
				   (x + secondXOff)*2, (y + secondYOff)*2);
        Ellipse2D.Double ellipse = new Ellipse2D.Double((x-len1)*2, (y-len2)*2,
														len1*4, len2*4);
        Shape rotated = (AffineTransform.getRotateInstance(ang1, x*2, y*2).
						 createTransformedShape(ellipse));
        g.draw(rotated);
    }


	/* Called when our display conditions have changed, but we still want to
	 * run on the current log.
	 */

	public void rerunLog() {
		System.out.println("Rerunning log");
        CrossInstance ci = CrossIO.instanceByIndex(0);
        if (ci == null)
            return;
        CrossFunc func = ci.functionWithName("Vision");
        assert(func != null);

        CrossCall cc = new CrossCall(this, func, currentLog);

        assert(ci.tryAddCall(cc));
	}

	/* Currently only called by the JComboBox, if we start adding more actions
	 * then we will need to update this accordingly.
	 */
	public void actionPerformed(ActionEvent e) {
		JComboBox cb = (JComboBox)e.getSource();
		String viewName = (String)cb.getSelectedItem();
		updateView(viewName);
	}

	/* Updates the image displayed on the bottom according to the user's
	 * selection.
	 */
	public void updateView(String viewName) {
		if (viewName == "Green") {
			currentBottom = GREEN_IMAGE;
		} else if (viewName == "White") {
			currentBottom = WHITE_IMAGE;
		} else if (viewName == "Orange") {
			currentBottom = ORANGE_IMAGE;
		} else if (viewName == "Edge") {
			currentBottom = EDGE_IMAGE;
		} else if (viewName == "Original") {
			currentBottom = ORIGINAL;
		} else {
			currentBottom = ORIGINAL;
		}
		repaint();
	}

    class DistanceGetter implements MouseListener {

      public void mouseClicked(MouseEvent e) {
        repaint();
      }

      public void mousePressed(MouseEvent e) {
      }

      public void mouseReleased(MouseEvent e) {
		  repaint();
      }

      public void mouseEntered(MouseEvent e) {}

      public void mouseExited(MouseEvent e) {}
    }

	class CheckBoxListener implements ItemListener {
		public void itemStateChanged(ItemEvent e) {
			int index = 0;
			Object source = e.getSource();
			if (source == showCameraHorizon) {
				index = 0;
			} else if (source == showFieldHorizon) {
				index = 1;
			} else if (source == debugHorizon) {
				index = 2;
			} else if (source == debugFieldEdge) {
				index = 3;
			} else if (source == debugBall) {
				index = 4;
				drawAllBalls = !drawAllBalls;
			} else if (source == showFieldLines) {
				index = -1;
				displayFieldLines = !displayFieldLines;
			}
			// flip the value of the parameter checked
			if (index >= 0) {
				if (displayParams[index] == 0) {
					displayParams[index] = 1;
				} else {
					displayParams[index] = 0;
				}
				adjustParams();
			} else {
				repaint();
			}
		}
	}

    @Override
    public void ioFinished(IOInstance instance) {}

    @Override
    public void ioReceived(IOInstance inst, int ret, Log... out) {
		System.out.println("IO received in Debug");
		if (out.length > GREEN_IMAGE) {
            Y8image green8 = new Y8image(width, height, out[GREEN_IMAGE].bytes);
            displayImages[GREEN_IMAGE] = green8.toBufferedImage();
        }

		if (out.length > WHITE_IMAGE) {
            Y8image white8 = new Y8image(width, height, out[WHITE_IMAGE].bytes);
            displayImages[WHITE_IMAGE] = white8.toBufferedImage();
        }

		if (out.length > ORANGE_IMAGE) {
            Y8image orange8 = new Y8image(width, height, out[ORANGE_IMAGE].bytes);
            displayImages[ORANGE_IMAGE] = orange8.toBufferedImage();
        }

		if (out.length > EDGE_IMAGE) {
			EdgeImage ei = new EdgeImage(width, height,  out[EDGE_IMAGE].bytes);
			displayImages[EDGE_IMAGE] = ei.toBufferedImage();
		}

        lines = new Vector<Double>();
        byte[] lineBytes = out[LINE_IMAGE].bytes;
        int numLines = lineBytes.length / (9 * 8);
        try {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(lineBytes));
            for (int i = 0; i < numLines; ++i) {
                lines.add(dis.readDouble()); // image coord r
                lines.add(dis.readDouble()); // image coord t
                lines.add(dis.readDouble()); // image coord ep0
                lines.add(dis.readDouble()); // image coord ep1
                lines.add((double)dis.readInt()); // hough index
                lines.add((double)dis.readInt()); // fieldline index
                lines.add(dis.readDouble()); // field coord r
                lines.add(dis.readDouble()); // field coord t
                lines.add(dis.readDouble()); // field coord ep0
                lines.add(dis.readDouble()); // field coord ep1
            }
        } catch (Exception e) {
            Logger.logf(Logger.ERROR, "Conversion to hough coord lines failed.");
            e.printStackTrace();
        }

        ccPoints = new Vector<Double>();
        byte[] pointBytes = out[8].bytes;
        int numPoints = pointBytes.length / (2 * 8);

        try {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(pointBytes));
            for (int i = 0; i < numPoints; i ++) {
                ccPoints.add(dis.readDouble()); // X coordinate
                ccPoints.add(dis.readDouble()); // Y coodinrate
            }
        } catch (Exception e) {
            Logger.logf(Logger.ERROR, "Conversion from bytes to center failed.");
            e.printStackTrace();
        }

		SExpr otree = out[ORANGE_IMAGE].tree();
        Y8image o = new Y8image(otree.find("width").get(1).valueAsInt(),
                                otree.find("height").get(1).valueAsInt(),
                                out[ORANGE_IMAGE].bytes);
        balls = out[BALL_IMAGE];



        debugImage = new DebugImage(width, height, out[DRAWING].bytes,
									   displayImages[ORIGINAL]);
        debugImageDisplay = debugImage.toBufferedImage();

		newLog = false;

        repaint();

    }

    @Override
    public boolean ioMayRespondOnCenterThread(IOInstance inst) {
        return false;
    }
}
