import java.io.*;
import java.util.*;

public class ScaleSkin {
    private static final double SCALE = 1.171875;

    public static void main(String[] args) {
        try {
            BufferedReader reader = new BufferedReader(new FileReader("Ehrling42sl.layout"));
            PrintWriter writer = new PrintWriter("Ehrling42sl.X.layout");
            String line;
            while ((line = reader.readLine()) != null) {
                StringTokenizer tok = new StringTokenizer(line, " ");
                StringBuffer buf = new StringBuffer();
                if (line.startsWith("Skin:")) {
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(transformRect(tok.nextToken(), 0));
                    writer.println(buf.toString());
                } else if (line.startsWith("Display:")) {
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(transformPoint(tok.nextToken(), 84));
                    String t;
                    while (tok.hasMoreTokens()) {
                        buf.append(" ");
                        buf.append(tok.nextToken());
                    }
                    writer.println(buf.toString());
                } else if (line.startsWith("Key:")) {
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(transformRect(tok.nextToken(), 168));
                    buf.append(" ");
                    buf.append(transformRect(tok.nextToken(), 168));
                    buf.append(" ");
                    buf.append(transformPoint(tok.nextToken(), 168));
                    writer.println(buf.toString());
                } else if (line.startsWith("Annunciator:")) {
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(tok.nextToken());
                    buf.append(" ");
                    buf.append(transformRect(tok.nextToken(), 84));
                    buf.append(" ");
                    buf.append(transformPoint(tok.nextToken(), 252));
                    writer.println(buf.toString());
                } else {
                    writer.println(line);
                }
            }
            writer.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private static String transformPoint(String p, int offset) {
        StringTokenizer tok = new StringTokenizer(p, ",");
        int x = Integer.parseInt(tok.nextToken());
        int y = Integer.parseInt(tok.nextToken());
        return Math.floor(x * SCALE) + "," + (Math.floor(y * SCALE) + offset);
    }

    private static String transformRect(String p, int offset) {
        StringTokenizer tok = new StringTokenizer(p, ",");
        int x = Integer.parseInt(tok.nextToken());
        int y = Integer.parseInt(tok.nextToken());
        int w = Integer.parseInt(tok.nextToken());
        int h = Integer.parseInt(tok.nextToken());
        return Math.floor(x * SCALE) + "," + (Math.floor(y * SCALE) + offset)
            + "," + Math.ceil(w * SCALE) + "," + Math.ceil(h * SCALE);
    }
}
