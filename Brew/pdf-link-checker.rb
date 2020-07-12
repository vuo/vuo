class PdfLinkChecker < Formula
  url "https://github.com/bootlin/pdf-link-checker.git",
      :revision => "17a18018bde81307a84dba09057feb39869a4a37"
  version '0'

  def install
    prefix.install Dir["*"]
  end

  test do
    system "#{prefix}/pdf-link-checker", "--help"
  end
end
