class RecurringTransaction {
    private int id;                 // PK
    private int userId;             // FK - 사용자 ID
    private String type;            // 유형 ("수입" 또는 "지출")
    private double amount;          // 금액
    private String category;        // 카테고리
    private String content;         // 메모
    private int dayOfMonth;         // 지정 날짜 (1-31)

    // DB 저장 전 (ID 제외)
    public RecurringTransaction(int userId, String type, double amount, String category, String content, int dayOfMonth) {
        this.userId = userId;
        this.type = type;
        this.amount = amount;
        this.category = category;
        this.content = content;
        this.dayOfMonth = dayOfMonth;
    }

    // DB에서 로드 시 (ID 포함)
    public RecurringTransaction(int id, int userId, String type, double amount, String category, String content, int dayOfMonth) {
        this.id = id;
        this.userId = userId;
        this.type = type;
        this.amount = amount;
        this.category = category;
        this.content = content;
        this.dayOfMonth = dayOfMonth;
    }

    // Getter 메서드
    public int getId() { return id; }
    public int getUserId() { return userId; }
    public String getType() { return type; }
    public double getAmount() { return amount; }
    public String getCategory() { return category; }
    public String getContent() { return content; }
    public int getDayOfMonth() { return dayOfMonth; }
}