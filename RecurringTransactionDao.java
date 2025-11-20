import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

public class RecurringTransactionDao {
    public boolean addRecurringTransaction(RecurringTransaction rt) {
        String sql = "INSERT INTO recurring_transactions (user_id, type, amount, category, content, day_of_month) VALUES (?, ?, ?, ?, ?, ?)";
        
        try (Connection conn = DatabaseManager.connect(); 
             PreparedStatement pstmt = conn.prepareStatement(sql)) {
            
            pstmt.setInt(1, rt.getUserId());
            pstmt.setString(2, rt.getType());
            pstmt.setDouble(3, rt.getAmount());
            pstmt.setString(4, rt.getCategory());
            pstmt.setString(5, rt.getContent());
            pstmt.setInt(6, rt.getDayOfMonth());
            
            pstmt.executeUpdate();
            return true;
            
        } catch (SQLException e) {
            System.err.println("DB 정기 거래 저장 오류: " + e.getMessage());
            return false;
        }
    }

    //정기 거래 내역 조회
    public List<RecurringTransaction> getAllRecurringTransactions(int userId) {
        List<RecurringTransaction> transactions = new ArrayList<>();
        String sql = "SELECT id, user_id, type, amount, category, content, day_of_month "
                   + "FROM recurring_transactions "
                   + "WHERE user_id = ? "
                   + "ORDER BY day_of_month ASC, type DESC";

        try (Connection conn = DatabaseManager.connect();
             PreparedStatement pstmt = conn.prepareStatement(sql)) {
            
            pstmt.setInt(1, userId);
            
            try (ResultSet rs = pstmt.executeQuery()) {
                while (rs.next()) {
                    RecurringTransaction rt = new RecurringTransaction(
                        rs.getInt("id"),
                        rs.getInt("user_id"),
                        rs.getString("type"),
                        rs.getDouble("amount"),
                        rs.getString("category"),
                        rs.getString("content"),
                        rs.getInt("day_of_month")
                    );
                    transactions.add(rt);
                }
            }
        } catch (SQLException e) {
            System.err.println("정기 거래 조회 중 DB 오류: " + e.getMessage());
        }
        return transactions;
    }
    
    //정기 거래 ID 조회 및 삭제
    public boolean deleteRecurringTransaction(int transactionId) {
        String sql = "DELETE FROM recurring_transactions WHERE id = ?";
        
        try (Connection conn = DatabaseManager.connect();
             PreparedStatement pstmt = conn.prepareStatement(sql)) {
            
            pstmt.setInt(1, transactionId);
            
            int affectedRows = pstmt.executeUpdate();
            return affectedRows > 0;
            
        } catch (SQLException e) {
            System.err.println("DB 정기 거래 삭제 오류: " + e.getMessage());
            return false;
        }
    }
}