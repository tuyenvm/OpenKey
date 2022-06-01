## Cách build cho macOS:
Vì một số lý do mà OpenKey không thể đưa lêp Mac App Store được, nếu các bạn không yên tâm về file build chính thức từ tác giả, các bạn có thể tải mã nguồn về tự build lấy OpenKey cho mình. 

Yêu cầu:
- macOS Mojave trở lên.
- XCode 10 trở lên.

Tải mã nguồn từ dự án OpenKey trên GitHub về, mở dự án OpenKey bằng cách vào:
<img width="808" alt="Screen Shot 2022-06-01 at 13 34 39" src="https://user-images.githubusercontent.com/7700801/171342552-46c391a7-9160-4677-bb8a-9d8cececcfe6.png">

Dự án OpenKey sẽ hiện ra:
<img width="1522" alt="Screen Shot 2022-06-01 at 13 37 36" src="https://user-images.githubusercontent.com/7700801/171343050-aa2f9629-4670-4c1e-a3c0-0c25acefa42d.png">

Giờ bạn vào menu Product -> Archive để Xcode tiến hành build OpenKey ở dạng Production (có thể dùng được), sau khi build xong hộp thoại sẽ xuất hiện, bấm vào Distribute App
<img width="1112" alt="Screen Shot 2022-06-01 at 13 45 50" src="https://user-images.githubusercontent.com/7700801/171344111-540b66cb-556f-48cf-ad3b-0f44ab208d1a.png">

Nó sẽ ra:
<img width="1112" alt="Screen Shot 2022-06-01 at 13 48 53" src="https://user-images.githubusercontent.com/7700801/171344529-12eed723-4e66-4818-8502-fa693b269dba.png">

Bấm Next để chọn nơi lưu trữ file Build. Vậy là xong, bạn đã có ứng dụng OpenKey.app của riêng bạn.

Lưu ý: trong quá trình build, nếu XCode bão lỗi `invalid in C99`, khả năng cao là ở trong file `MJAccessibilityUtils.m`, dòng 26 `isEnabled = AXAPIEnabled();`, bạn lên đầu file thêm khai báo `extern BOOL AXAPIEnabled();` là sẽ build được.

Chúc các bạn Build OpenKey cho macOS thành công!
