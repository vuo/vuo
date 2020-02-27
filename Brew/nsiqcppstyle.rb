class Nsiqcppstyle < Formula
  url "https://github.com/kunaltyagi/nsiqcppstyle.git",
      :revision => "1774ad4dce4bf2cf9b45433a62457ec1eda828bb"
  version "0.2.2.13"

  def install
    prefix.install Dir["*"]
  end

  test do
    system "#{prefix}/nsiqcppstyle", "--help"
  end
end
