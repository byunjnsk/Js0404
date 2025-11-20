import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;
import java.util.HashMap;
import java.util.Map;

public class RecurringView extends JDialog implements ActionListener {

    private final CalendarSwing parent;
    private final User currentUser;
    private final RecurringTransactionDao recurringDao;
    
    private DefaultTableModel tableModel;
    private JTable transactionsTable;

    private JComboBox<String> typeCombo;
    private JComboBox<String> categoryCombo;
    private JTextField contentField;
    private JTextField amountField;
    private JComboBox<Integer> dayOfMonthCombo;
    
    private JButton addButton;
    private JButton deleteButton;

    private final Map<String, String[]> categories = new HashMap<>();

    public RecurringView(CalendarSwing owner, User user, RecurringTransactionDao dao) {
        super(owner, "정기 거래 관리", true);
        this.parent = owner;
        this.currentUser = user;
        this.recurringDao = dao;

        initCategories();
        
        setSize(800, 500);
        setLayout(new BorderLayout());
        setLocationRelativeTo(owner);
        
        String[] columnNames = {"ID", "유형", "카테고리", "내용", "금액", "반복일"};
        tableModel = new DefaultTableModel(columnNames, 0) {
            @Override
            public boolean isCellEditable(int row, int column) {
                return false;
            }
        };
        transactionsTable = new JTable(tableModel);
        transactionsTable.getColumn("ID").setMinWidth(0);
        transactionsTable.getColumn("ID").setMaxWidth(0);
        transactionsTable.getColumn("ID").setWidth(0);

        JScrollPane tableScrollPane = new JScrollPane(transactionsTable);
        add(tableScrollPane, BorderLayout.CENTER);

        JPanel southPanel = new JPanel(new BorderLayout());
        JPanel inputPanel = createInputPanel();
        southPanel.add(inputPanel, BorderLayout.NORTH);
        
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        addButton = new JButton("정기 거래 추가");
        deleteButton = new JButton("선택 항목 삭제");
        addButton.addActionListener(this);
        deleteButton.addActionListener(this);
        buttonPanel.add(addButton);
        buttonPanel.add(deleteButton);
        southPanel.add(buttonPanel, BorderLayout.SOUTH);
        
        add(southPanel, BorderLayout.SOUTH);

        loadRecurringTransactions();
    }

    //카테고리 맵 초기화
    private void initCategories() {
        categories.put("지출", new String[]{"식비", "교통", "생활/쇼핑", "문화/여가", "건강/의료", "경조사/모임", "교육/자기개발", "기타"});
        categories.put("수입", new String[]{"근로 소득", "부가 소득", "금융 소득", "기타 소득"});
    }
    
    //정기 거래 입력 패널
    private JPanel createInputPanel() {
        JPanel inputPanel = new JPanel(new GridLayout(3, 4, 10, 5));
        inputPanel.setBorder(BorderFactory.createTitledBorder("새 정기 거래 입력"));

        typeCombo = new JComboBox<>(new String[]{"지출", "수입"});
        categoryCombo = new JComboBox<>();
        contentField = new JTextField();
        amountField = new JTextField();
        
        dayOfMonthCombo = new JComboBox<>();
        for (int i = 1; i <= 31; i++) {
            dayOfMonthCombo.addItem(i);
        }

        typeCombo.addActionListener(e -> updateCategoryCombo());
        updateCategoryCombo(); // 초기 카테고리 목록 설정

        inputPanel.add(new JLabel("유형:"));
        inputPanel.add(typeCombo);
        inputPanel.add(new JLabel("카테고리:"));
        inputPanel.add(categoryCombo);
        inputPanel.add(new JLabel("반복일 (매월):"));
        inputPanel.add(dayOfMonthCombo);
        inputPanel.add(new JLabel("금액:"));
        inputPanel.add(amountField);
        inputPanel.add(new JLabel("내용:"));
        inputPanel.add(contentField);
        inputPanel.add(new JLabel("")); // 여백
        inputPanel.add(new JLabel("")); // 여백

        return inputPanel;
    }

    //콤보박스 업데이트
    private void updateCategoryCombo() {
        String selectedType = (String) typeCombo.getSelectedItem();
        categoryCombo.removeAllItems();
        if (selectedType != null) {
            String[] cats = categories.get(selectedType);
            if (cats != null) {
                for (String cat : cats) {
                    categoryCombo.addItem(cat);
                }
            }
        }
    }
    
    //DB에서 정기거래 내역 로드
    private void loadRecurringTransactions() {
        tableModel.setRowCount(0); 
        List<RecurringTransaction> list = recurringDao.getAllRecurringTransactions(currentUser.getUserId());
        
        for (RecurringTransaction rt : list) {
            Object[] row = {
                rt.getId(),
                rt.getType(),
                rt.getCategory(),
                rt.getContent(),
                String.format("%,.0f", rt.getAmount()),
                rt.getDayOfMonth() + "일"
            };
            tableModel.addRow(row);
        }
    }

    //입력 필드 초기화
    private void clearInputFields() {
        typeCombo.setSelectedIndex(0);
        updateCategoryCombo();
        contentField.setText("");
        amountField.setText("");
        dayOfMonthCombo.setSelectedIndex(0); // 1일로 초기화
    }
    
    //정기 거래 DB에 추가
    private void addRecurringTransaction() {
        try {
            String type = (String) typeCombo.getSelectedItem();
            String category = (String) categoryCombo.getSelectedItem();
            String content = contentField.getText();
            int dayOfMonth = (int) dayOfMonthCombo.getSelectedItem();
            
            if (content == null || content.trim().isEmpty()) {
                 JOptionPane.showMessageDialog(this, "상세 내용을 입력하세요.");
                 return;
            }

            String amountText = amountField.getText();
            double amount = Double.parseDouble(amountText);
            
            if (amount <= 0) {
                 JOptionPane.showMessageDialog(this, "금액은 0보다 커야 합니다.");
                 return;
            }

            RecurringTransaction newRt = new RecurringTransaction(
                currentUser.getUserId(),
                type,
                amount,
                category,
                content,
                dayOfMonth
            );

            if (recurringDao.addRecurringTransaction(newRt)) {
                JOptionPane.showMessageDialog(this, "정기 거래가 성공적으로 추가되었습니다.");
                loadRecurringTransactions();
                clearInputFields();
                
                if (parent != null) {
                    parent.loadMonthData(); 
                    
                    String selectedDate = parent.getCurrentSelectedDate();
                    if (selectedDate != null) {
                         parent.updateDetailsPanel(selectedDate);
                    }
                }
                
            } else {
                JOptionPane.showMessageDialog(this, "정기 거래 추가에 실패했습니다.", "오류", JOptionPane.ERROR_MESSAGE);
            }

        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "금액은 숫자로만 입력하세요.", "오류", JOptionPane.ERROR_MESSAGE);
        } catch (Exception e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog(this, "입력 중 오류 발생: " + e.getMessage(), "오류", JOptionPane.ERROR_MESSAGE);
        }
    }

    //정기거래 DB에서 삭제
    private void deleteRecurringTransaction() {
        int selectedRow = transactionsTable.getSelectedRow();

        if (selectedRow == -1) {
            JOptionPane.showMessageDialog(this, "삭제할 정기 거래 항목을 테이블에서 선택하세요.");
            return;
        }

        int modelRow = transactionsTable.convertRowIndexToModel(selectedRow);
        int transactionId = (int) tableModel.getValueAt(modelRow, 0); 

        int confirm = JOptionPane.showConfirmDialog(this,
            "정말로 이 정기 거래를 삭제하시겠습니까?", "삭제 확인", JOptionPane.YES_NO_OPTION);

        if (confirm == JOptionPane.YES_OPTION) {
            if (recurringDao.deleteRecurringTransaction(transactionId)) {
                JOptionPane.showMessageDialog(this, "정기 거래가 성공적으로 삭제되었습니다.");
                loadRecurringTransactions();
                if (parent != null) {
                    parent.loadMonthData(); 
                    if (parent.getCurrentSelectedDate() != null) {
                         parent.updateDetailsPanel(parent.getCurrentSelectedDate());
                    }
                }
            } else {
                JOptionPane.showMessageDialog(this, "정기 거래 삭제에 실패했습니다.", "오류", JOptionPane.ERROR_MESSAGE);
            }
        }
    }

    @Override
    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == addButton) {
            addRecurringTransaction();
        } else if (e.getSource() == deleteButton) {
            deleteRecurringTransaction();
        }
    }
}
