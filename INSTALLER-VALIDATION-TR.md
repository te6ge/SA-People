# 0.12.5 beta — kurulum doğrulaması, 23 Ekim 2026

EXE ve Install.cmd, gerçek oyun klasörüne dokunulmadan sahte oyun klasörlerinde sınandı. Tam Build.cmd derlemesi, mevcut oyun mantığı testleri ve kurulum testinin 61 kontrolü geçti.

## Düzeltilenler

- Install.cmd, xcopy ve bazı copy hatalarını kontrol etmiyor; kopyalama başarısız olsa da başarı mesajı ve çıkış kodu 0 üretebiliyordu. Salt okunur hedef dosyayla yeniden üretildi. Artık tüm gerekli kopyalama/yedekleme adımları kontrol ediliyor; hata kodu 3 ve açık başarısızlık mesajı var.
- Dosyanın gerekli bir klasörün yerini aldığı durum, kopyalama başlamadan yakalanıyor. Eksik paket dosyaları da önceden kontrol ediliyor.
- EXE kurulum gövdesinde gömülü Windows yürütme bildirimi yoktu. Setup adından dolayı doğrudan çalıştırma WinError 740 verebiliyordu. Build.cmd artık asInvoker bildirimini gömüyor. Normal yazılabilir klasörde yönetici olmadan sessiz test çalışıyor; korumalı klasörlerde yazma izni hâlâ gerekir.
- EXE yedekleme/dizin oluşturma hataları artık yutulmuyor; dosyalar kapatıldıktan sonraki yazma hataları da kontrol ediliyor. Bozuk paket yolları ve taşan boyut/konum alanları reddediliyor. Sessiz kurulum desteklenmeyen EXE boyutunu onaysız kabul etmiyor.
- CMD kurulumu artık loader lisansını SA-People/licenses içine de kopyalıyor.
- Kaynak ZIP'e loader, lisans, release-template, ayar örneği, rehber ve kurulum testleri eklendi. Kaynakta ayrı sağlanan ses/resim klasörleri yoksa paketleyici bunları zorunlu saymıyor; oyunun mantık testleri için mevcut BUILDING.md medya gereksinimleri geçerli.
- README, aynı adlı paket ses/resim dosyalarının güncellendiğini ve farklı adlı kullanıcı eklerinin korunduğunu doğru biçimde açıklıyor.

## Test kapsamı

Her iki kurulum yolu için:

- Loader yok; 64 KB stok vorbisFile.dll var; mevcut dinput8.dll var; büyük vorbisFile.dll loader var.
- Kopyalanan ASI ve tüm veri dosyalarının içerik karşılaştırması; loader lisansı ve mevcut loader dosyasının korunması.
- Türkçe karakter, boşluk ve & içeren hedef klasör.
- Eski ASI yedeği; people.dat, people.dat.bak ve settings.ini korunması/yedeği; özel ses ve resimlerin korunması.
- İkinci kurulum ve ayrı yedek oluşturulması.
- Klasör yerine dosya, salt okunur hedef, gta_sa.exe bulunmaması ve desteklenmeyen EXE boyutu.
- EXE için dizin dışına taşan/absolute paket yolu, taşan dosya konumu ve bilinmeyen kayıt modu.
- Kaynak paketin loader/lisans/şablon/test içermesi.

Loader x86 PE olarak doğrulandı ve yerel Ultimate-ASI-Loader-9.7.4.zip içindeki dinput8.dll ile bayt bayt aynı:

`EC2F4824ECA58DD40F425756A4A7CEC77B8E381F21D11D7C846EC4B339B617AB`

## Tekrar çalıştırma

1. Build.cmd ile derle.
2. MakeRelease.ps1 ile paketle.
3. Python 3 ile tests/InstallerTests.py çalıştır.

Rapor ve yakalanan stdout/stderr dosyaları build/installer-tests-YYYYMMDD-HHMMSS altında tutulur. Başarısız dosya kopyalamaları bazı önceki dosyaları güncellemiş olabilir; kurulum tam işlem geri alma mekanizması değildir. Eski ASI kurulumdan önce yedeklenir, kayıt ve kişisel ayarlar üzerine yazılmaz.

Claude'un gördüğü birebir “File creation error” satırının önceki test ortamı elde olmadığı için aynı metin doğrulanamadı; salt okunur hedefteki kopyalama hatasının başarı gibi raporlanması yeniden üretildi ve giderildi. Temiz testlerde stderr uyarısı çıkmadı.

Oyun açılışı, ASI'nin oyun içinde yüklenmesi ve görsel klasör seçme penceresi bu testte elle denenmedi. Testler kurulumun sessiz modu ve CMD yolu üzerindedir.
