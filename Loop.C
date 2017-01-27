// Loop.C -- root macro
// Author : Mizukoshi Keita
// 2017 Jan. 26

void Loop(){

	const Double_t THRESHOLD = -1.0e-2; //Volt
	const Int_t BIN_NUM = 10000;


	//Init
	TString header = "./data/yjsn_";
	TString footer = "_ch1.root";
	stringstream ss;
	Int_t fileNumber=0;
	ss << header << fileNumber << footer;

	TH1D* histCharge = new TH1D("charge","charge",10000,-1.0e-12,100.0e-12);

	// Loop
	while(TFile* f1 = TFile::Open(ss.str().c_str())){
		TGraph* g1 = dynamic_cast<TGraph*>(f1->Get("graph"));

		Double_t* x = g1->GetX();
		Double_t* y = g1->GetY();


		//define baseline
		Double_t baseline;
		for(int i=0;i<(BIN_NUM/10);++i){
			baseline += y[i];
		}
		baseline /= (BIN_NUM/10);
		
		Double_t integral = 0;
		for(int i=0;i<BIN_NUM;++i){
			if((y[i]-baseline)<THRESHOLD) integral += y[i]-baseline;
		}

		histCharge->Fill( TMath::Abs(integral *= (4.0e-10/50.0) )); //50.0 Ohm

		// prepare for next loop
		ss.str("");
		ss.clear(stringstream::goodbit);
		++fileNumber;
		ss << header << fileNumber << footer;
		delete g1;
		delete f1;
	}
	histCharge->SetTitle("Charge;C;count");
	histCharge->Draw();

}
