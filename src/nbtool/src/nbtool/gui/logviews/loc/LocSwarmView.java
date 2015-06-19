package nbtool.gui.logviews.loc;

//imports
	import java.awt.*;
	import java.awt.geom.*;
	import java.awt.event.*;
	import javax.swing.*;

	import java.io.ByteArrayInputStream;
	import java.io.IOException;
	import java.io.InputStream;
	import java.util.Vector;

	import com.google.protobuf.Message;

	import messages.RobotLocationOuterClass.*;
	import messages.ParticleSwarmOuterClass.ParticleSwarm;
	import messages.ParticleSwarmOuterClass.Particle;
	import messages.Vision.*;

	import nbtool.data.Log;
	import nbtool.data.SExpr;
	import nbtool.gui.logviews.misc.ViewParent;

public class LocSwarmView extends ViewParent implements ActionListener {

	@Override
	public void setLog(Log newlog) {
		log = newlog;

		RobotLocation naoLoc;
		ParticleSwarm naoSwarm;
		FieldLines naoFieldLines;

		float naoX, naoY, naoH;

		try {
			naoLoc = RobotLocation.parseFrom(log.bytesForContentItem(0));
			naoX = naoLoc.getX();
			naoY = naoLoc.getY();
			naoH = naoLoc.getH();
			naoPlayer.moveTo(naoX,naoY, naoH);

			naoSwarm = ParticleSwarm.parseFrom(log.bytesForContentItem(1));
			for(int i=0; i<naoSwarm.getParticleCount(); i++) {
				RobotLocation currentNaoSwarm = naoSwarm.getParticle(i).getLoc();
				pWeight = naoSwarm.getParticle(i).getWeight();
				NaoParticle temp = new NaoParticle();
				temp.moveTo(currentNaoSwarm.getX(),currentNaoSwarm.getY());
				naoParticles.add(temp);
			}
			
			naoFieldLines = FieldLines.parseFrom(log.bytesForContentItem(2));
			for(int i=0; i<naoFieldLines.getLineCount(); i++) {
				FieldLine curFieldLine = naoFieldLines.getLine(i);
				GeoLine temp = new GeoLine(
								curFieldLine.getInner().getR(),
								curFieldLine.getInner().getT(),
								curFieldLine.getInner().getEp0(),
								curFieldLine.getInner().getEp1(),
								0.0,
								0.0);
				naoLines.add(temp);
			}
            /* numLines = 1; */
			/* for(int i=0; i<1; i++) { */
			/* 	naoLines[i].r = FieldConstants.CENTER_FIELD_X; */
			/* 	naoLines[i].t = 0; */
			/* 	naoLines[i].end0 = 100; */
			/* 	naoLines[i].end1 = 200; */
			/* 	naoLines[i].houghIndex = 0.0; */
			/* 	naoLines[i].fieldIndex = 0.0; */
			/* } */
		} catch(Exception e) {
			e.printStackTrace();
		}
	}

	public LocSwarmView() {
		super();
		if(shouldFlip) {
			flip = new JButton("unflip");
		} else { flip = new JButton("flip"); }
		flip.addActionListener(this);
		flip.setPreferredSize(new Dimension(70,25));
		this.add(flip);
	}

	protected void useSize(Dimension s) {
		sp.setBounds(0,0,s.width,s.height);
	}

	public void actionPerformed(ActionEvent e) {
		if(e.getSource() == flip) {
			if(shouldFlip == true) {
				shouldFlip = false;
				flip.setText("flip");
				repaint();
			} else { 
				shouldFlip = true;
				flip.setText("unflip");
				repaint();
			}
		}
	}

	Field f = new Field();
	NaoRobot naoPlayer = new NaoRobot();
	Vector<NaoParticle> naoParticles = new Vector<NaoParticle>();
	Vector<GeoLine> naoLines = new Vector<GeoLine>();
	private JButton flip;
	private JScrollPane sp;
	public static float pWeight;
	public static boolean shouldFlip = false;
	public static final Dimension fieldDimension = new Dimension(
								(int)FieldConstants.FIELD_WIDTH,
								(int)FieldConstants.FIELD_HEIGHT);

	@Override
	public void paintComponent(Graphics g) {
		Graphics2D g2 = (Graphics2D) g;
		f.drawField(g2);
		naoPlayer.drawNao(g2,shouldFlip);
		if(!naoParticles.isEmpty()) {
			for(int i=0; i<naoParticles.size(); i++) {
				naoParticles.get(i).drawParticle(g2,pWeight,shouldFlip);
			}
		}
		if(!naoLines.isEmpty()) {
			for(int i=0; i<naoLines.size(); i++) {
				naoLines.get(i).draw(g2);
			}
		}
	}
}
