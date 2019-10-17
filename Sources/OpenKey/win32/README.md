# OpenKey for Windows
[![GitHub release](https://img.shields.io/github/v/release/tuyenvm/OpenKey.svg)](https://github.com/tuyenvm/OpenKey/releases/latest)
- OpenKey phiên bản cho Windows bắt đầu từ bản 1.5, có tất cả các tính năng của bản macOS.  
- OpenKey mặc định chạy ở chế độ `Administrator` để có thể dễ dàng gõ Tiếng Việt trong **game**. Mã nguồn đã được phát hành công khai dưới giấy phép **GPL** nên các bạn hoàn toàn yên tâm về tính minh bạch của chương trình, không keylog, không backdoor,...
- OpenKey for Windows hỗ trợ từ Windows Vista trở lên, Windows XP thì chưa có cơ hội để test, Windows 2000 trở về trước thì chắc chắn không thể hỗ trợ rồi.
![Giao diện chính](https://raw.githubusercontent.com/tuyenvm/tuyenvm.github.io/master/images/win32_2.0_1.png "Main UI")



## Tính năng
Mặc định, OpenKey cho phép gõ với chế độ bỏ dấu tự do, không cần phải bỏ dấu sau nguyên âm.
* Hỗ trợ kiểu gõ:
	* Telex
	* VNI
	* Simple Telex
	* Có thể yêu cầu thêm kiểu gõ.
* Hỗ trợ bảng mã:
	* Unicode dựng sẵn
	* TCVN3 (ABC)
	* VNI Windows
	* Unicode Tổ hợp
	* Vietnamese locale CP 1258
* Chính tả:
	* Bật tắt kiểm tra chính tả
	* Tự khôi phục phím với từ sai
	* Cho phép chữ "z w j f" làm phụ âm đầu
	* Tạm tắt chính tả bằng phím Ctrl
	* Tạm tắt OpenKey bằng phím Alt
* Tùy chọn:
	* Tự cấu hình phím tắt chuyển Anh - Việt
	* Bỏ dấu kiểu mới (oà, uý) và kiểu cũ (òa úy)
	* Sửa lỗi gợi ý trên thanh địa chỉ trình duyệt, Excel
	* Hiện bảng điều khiển khi khởi động
	* Hai loại biểu tượng trên system tray (đồng bộ với Win10)
	* Khởi động cùng Windows.
	* Chạy với chế độ Admin.
	* Sửa lỗi trên các trình duyệt Chromium
	* Hỗ trợ các ứng dụng metro trên Windows 10
* Công cụ và tính năng nâng cao:
	* Loại trừ ứng dụng thông minh
	* Tự ghi nhớ bảng mã theo ứng dụng
	* Viết Hoa chữ cái đầu câu
	* Gõ nhanh (cc=ch, gg=gi, kk=kh, nn=ng, qq=qu, pp=ph, tt=th)
	* Gõ tắt và công cụ soạn gõ tắt.
	* Chuyển mã với nhiều tùy chọn.
	* Gõ tắt phụ âm đầu: f->ph, j->gi, w->qu
	* Gõ tắt phụ âm cuối: g->ng, h->nh, k->ch

## Cài đặt
- Vào [đây](https://github.com/tuyenvm/OpenKey/releases/latest) tải file `.zip` phiên bản mới nhất về máy, giải nén vào nơi bất kỳ.
- Nhấp đúp vô `OpenKey.exe` để chạy chương trình.
- Lưu ý, khi chạy chương trình lần đầu, Windows sẽ mở một hộp thoại để hỏi bạn, bấm `yes` để đồng ý OpenKey chạy với quyền `admin`
- Xong rồi một biểu tượng chữ `V` sẽ hiện trên system tray, đúp click vô đó để mở bảng điều khiển, cấu hình những thứ bạn cần.
- Xong.
## Một số hình ảnh khác
![Menu](https://raw.githubusercontent.com/tuyenvm/tuyenvm.github.io/master/images/win32_5.png "Menu")
![Bảng gõ tắt](https://raw.githubusercontent.com/tuyenvm/tuyenvm.github.io/master/images/win32_2.png "Macro table")
![Công cụ chuyển mã](https://raw.githubusercontent.com/tuyenvm/tuyenvm.github.io/master/images/win32_3.png "Convert tool")

## Thông tin
- Dự án OpenKey ra đời vào ngày 18/01/2019. OpenKey được viết lại hoàn toàn mới bởi tác giả Mai Vũ Tuyên. Mục đích ra đời của OpenKey là để khắc phục lỗi gạch chân khó chịu do cơ chế `preedit` gây ra trên hệ điều hành macOS
- Phiên bản đầu tiên cho hệ điều hành macOS được phát hành trên Github vào ngày 01/02/2019  
Trải qua nhiều lần cập nhật và phát triển, ngày 31/07/2019, OpenKey cho macOS được mọi người biết đến qua trang tinhte.vn  
- Với nhiều ý kiến đóng góp từ cộng đồng, OpenKey cho macOS phát triển tới phiên bản 1.5 đã đầy đủ các tính năng của một bộ gõ chuẩn, tiện lợi.  
- Ngày 19/09/2019 tác giả đã "port" OpenKey cho hệ điều hành Windows, phát triển song song với phiên bản cho macOS.  
- Phiên bản OpenKey cho Linux vẫn đang âm thầm phát triển, vì Linux có hệ thống GUI phức tạp, API "lạ" nên đòi hỏi thời gian nhiều hơn cho việc phát triển.

## Mã nguồn
- Mã nguồn cho Windows được build thành công với Microsoft Visual Studio 2017.
## Giấy phép
OpenKey for Windows được phát hành dưới giấy phép GPL, điều này nghĩa là bạn hoàn toàn có thể tự do tải mã nguồn về chỉnh sửa, cải tiến theo mục đích của bạn. Nếu bạn tái phân phối bản cải tiến của bạn, bạn **phải** công bố mã nguồn và credit OpenKey bản gốc.

## Một điều nữa
Đừng quên ủng hộ tác giả bằng cách mua ly cafe cho tác giả tỉnh ngủ nhé: [Gửi cafe](https://paypal.me/tuyenmai)  
Hoặc trực tiếp qua ví momo:   
![Donate by momo](https://tuyenvm.github.io/images/momo.png "Momo").   
Cảm ơn các bạn.