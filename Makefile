default: dnsedit

deploy: dnsedit
	cp dnsedit /usr/local/www/cgi-bin

dnsedit: dnsedit.cc
	g++ -o dnsedit dnsedit.cc
