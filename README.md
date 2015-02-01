damageDAO.java

package damageDB;

import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.SQLRecoverableException;
import java.sql.Statement;
import java.util.concurrent.CountDownLatch;

public class damageDAO implements Runnable{
	private CountDownLatch startSignal;
	public damageDAO(CountDownLatch startSignal) {
		this.startSignal = startSignal;
	}

	@Override
	public void run() {
		try {
			startSignal.await();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		DBUtil db = new DBUtil();
		System.out.println(Thread.currentThread().getName() + " start to connect.");
		Connection conn = null;
		while(true){			
			try {
				conn = db.getConnection();
				System.out.println(Thread.currentThread().getName() + " connect succeed!");
				String sql1 = "declare \r\n"
						+ " v_cnt number; \r\n"
						+ " begin \r\n"
						+ " select count(*) into v_cnt from user_tables where table_name = upper('damage_mytable');\r\n"
						+ " if v_cnt > 0 then \r\n"
						+ " execute immediate 'drop table damage_mytable purge'; \r\n"
						+ " end if; \r\n"
						+ " execute immediate 'create table damage_mytable(aa number(9), bb number(9), cc varchar(10))';\r\n"
						+ " execute immediate 'insert into /*+APPEND*/ damage_mytable select round(dbms_random.value(1000,9999),0),\r\n"
						+ " round(dbms_random.value(10000,999999),0), dbms_random.string(''A'',10)\r\n"
						+ " from dual connect by level < 1000001';\r\n"
						+ " commit;\r\n"
						+ " end;\r\n";
				String sql2 = "select * from damage_mytable";
				
				Statement st = null;
				st = conn.createStatement();
				System.out.println(Thread.currentThread().getName() + " start to execute sql1.");
				System.out.println(Thread.currentThread().getName() + " execute result:" + st.execute(sql1));
				
				st.close();
				st = conn.createStatement();
				
				ResultSet rs = null;
				System.out.println(Thread.currentThread().getName() + " start to execute sql2.");
				rs = st.executeQuery(sql2);
				while(rs.next()){}
				System.out.println(Thread.currentThread().getName() + " end sql2. Size: " + rs.getRow());
				while(true){
					
				}
			} catch (SQLRecoverableException e) {
				System.out.println("Connect failed, connect again!");
			} catch (SQLException e) {
				e.printStackTrace();
			}
		}
	}
	
	public static void main(String[] args) {
		CountDownLatch startSignal = new CountDownLatch(1);
		int count = 1;
		while(count-- > 0){
			damageDAO dd = new damageDAO(startSignal);
			new Thread(dd).start();
		}
		System.out.println("Start to connect!");
		startSignal.countDown();
	}

}

db.property
dbtype=oracle
dbhost=192.168.221.2
dbport=1521
dbname=orcl
dbusername=wuhao
dbpassword=wuhao





DBUtil.java
package damageDB;


import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.util.Properties;

public class DBUtil {
	
	public DBUtil(){}

	private String dbtype;

	private String dbhost;

	private String dbport;

	private String dbname;

	private String dbusername;

	private String dbpassword;

	/**
	 * 得到数据库连接
	 * @return
	 */
	public Connection getConnection() {
		Connection conn = null;

		Properties prop = new Properties();

		try {
			prop.load(getClass().getClassLoader().getResourceAsStream("damageDB\\db.property"));

			dbtype = prop.getProperty("dbtype");

			dbhost = prop.getProperty("dbhost");

			dbport = prop.getProperty("dbport");

			dbname = prop.getProperty("dbname");

			dbusername = prop.getProperty("dbusername");

			dbpassword = prop.getProperty("dbpassword");

			conn = createConnection();

		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		return conn;
	}

	private Connection createConnection() throws ClassNotFoundException,
			SQLException {
		Connection conn = null;
		if ("oracle".equals(dbtype)) {
			Class.forName("oracle.jdbc.driver.OracleDriver");
			// 拼接数据库url
			StringBuffer s = new StringBuffer("jdbc:oracle:thin:@");
			s.append(dbhost);
			s.append(":");
			s.append(dbport);
			s.append(":");
			s.append(dbname);

			conn = DriverManager.getConnection(s.toString(), dbusername,
					dbpassword);

		}
		return conn;
	}
	public static void main(String[] args) {
		DBUtil  dbutil = new DBUtil();
		System.out.println(dbutil.getConnection());
	}
}


