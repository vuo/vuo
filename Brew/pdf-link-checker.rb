class PdfLinkChecker < Formula
  url "https://github.com/smokris/pdf-link-checker.git",
      :revision => "fe95acee4aab440a1e335d023171471340a09544"
  version '0'

  def install
    prefix.install Dir["*"]
  end

  test do
    system "#{prefix}/pdf-link-checker", "--help"
  end
end
