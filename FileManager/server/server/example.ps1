# �����������
Write-Host "����� ���������� � PowerShell!"

# ����� ���������� � ������� ������������
$user = $env:USERNAME
Write-Host "�� ����� ���: $user"

# ����� ���������� �� ������������ �������
$os = Get-WmiObject Win32_OperatingSystem
Write-Host "������������ �������: $($os.Caption) $($os.Version)"