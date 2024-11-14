# First-LKM-VFSDemo

Output:

đầu ra của file test.c:

![image](https://github.com/user-attachments/assets/055ce889-4985-454c-8095-4edb74fbe214)

kernel log:

![image](https://github.com/user-attachments/assets/b1021154-a685-4bcc-b334-bea37017ec70)

quá trình đồng bộ hóa nếu các tiến trình truy cập vào file ebbchar cùng một thời điểm:
tiến trình 1 (đang truy cập vào ebbchar file):

![image](https://github.com/user-attachments/assets/043baacd-8cb0-45a2-85d9-f7028c970832)

tiến trình 2 (bị chặn truy cập vào ebbchar file cho đến khi tiến trình 1 kết thúc và giải phóng mutex):

![image](https://github.com/user-attachments/assets/69c3fca3-a236-4b80-87d1-0fcbdb68521f)

tiến trình 2 (sau khi tiến trình 1 kết thúc và giải phóng mutex):

![image](https://github.com/user-attachments/assets/7e298b5a-d7df-4b14-bd24-943c9de4acd4)
