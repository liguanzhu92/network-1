import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.HashSet;
import java.util.Set;

public class FindDuplicates {

    public static void main(String[] args) {
        Set<String> set = new HashSet<String>();
        try {
            FileInputStream fstream = new FileInputStream("seq_num");
            BufferedReader br = new BufferedReader(new InputStreamReader(fstream));

            String strLine;
            while ((strLine = br.readLine()) != null) {
                String[] s = strLine.split("\\s+");
                if(set.contains(s[1])) {
                    String e = s[0] + " " + s[1];
                    System.out.println(e);
                } else {
                    set.add(s[1]);
                }
            }

            br.close();
        } catch (Exception e) {

        }
    }
}
