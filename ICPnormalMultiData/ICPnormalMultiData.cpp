// ICPnormalMultiData.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "opencv.h"
#include "ICPnormal_index.h"
#include "glview.h"

#define FILEOUTPUT 1; //ファイル出力するかどうか
#define GLVIEW 1; //OpenGLで表示するかどうか

#pragma region // --- mainで使うグローバル変数 ---

const int fileTotal = 5; //処理するCSVファイルの総数
const int rows = 509; //データ点の数
char dir[] = "sample"; //ディレクトリ名

char filedir[] = "./data"; //入力点群
char outdir[] = "./data"; //入力点群

int fileCount = 0; //現在処理中のファイル番号のカウンタ
int pointCount = 0; //現在処理中のGLpointsのカウンタ
float pointSize = 3.0;

int model_rows[fileTotal];
int data_rows[fileTotal];
const int cols = 3;
#pragma endregion

#pragma region // --- OpenGL 変数 ---

/// ---設定--- ///
float cubeSize = 0.008; //立方体の一辺の長さ
float angle = 0.0; //立方体の回転角
float velocity = 0.002; //移動速度
GLdouble nearZ = 3.0, farZ = 6.0; //点群がどの距離の間にあるか
float axisSize = 0.25; //座標軸の長さ
float axisThick = 0.005; //座標軸の太さ
int window_w = 1000; //ウインドウサイズ
int window_h = 1000; //ウインドウサイズ
/// ---ここまで--- ///

//GLfloat * points[];
GLfloat * allpoints;
int window1;
float cubeSize_;
float velocityKey = 0.02;
#pragma endregion


//////////関数プロトタイプ宣言/////////////
cv::Mat csvread(const char* filename, const int rows, const int cols);


//////////main関数/////////////
int main(int argc, char** argv){
	
	cubeSize_ = cubeSize/2.;

	int i=0, j=0, k=0;
	clock_t start_time_total,end_time_total;
	clock_t start_time[fileTotal];
	clock_t end_time[fileTotal];

	//char * filename[fileTotal];
	//並列用
	char * model_filename[fileTotal];
	char * data_filename[fileTotal];

	//Mat shape[fileTotal];
	Mat shape_reg[fileTotal];
	//Mat shape_temp[fileTotal];
	Mat shape_fixed[fileTotal];

	//並列用
	Mat model_shape[fileTotal];
	Mat data_shape[fileTotal];
	Mat shape_temp[fileTotal][fileTotal];

	Mat my_model_corr[fileTotal];
	int myIndex[fileTotal][rows];
	float myDist[fileTotal][rows];
	RT<float> my_rt[fileTotal];

	Mat_<float> model_mean;

	//modelファイルのデータ数
	//model_rows = 16128;
	//dataファイルのデータ数
	//data_rows = 16128;
	
	start_time_total = clock();
	cout << "-------------" << endl;
	cout << "ICP Algorithm" << endl;
	cout << "-------------" << endl;

#pragma omp parallel for
	for(fileCount=0;fileCount<fileTotal;fileCount++)
	{

#pragma region // --- 点群のCSVファイルをcv::Matに取り込む ---
		if(fileCount>=1){
			///model
			//csvファイル名
			model_filename[fileCount] = (char *)malloc(sizeof(char *) * 100);
			//sprintf(model_filename[fileCount],"%s/%s/%d.csv",filedir,dir,fileCount);
			sprintf(model_filename[fileCount],"%s/%s/points%02d.csv",filedir,dir,fileCount);
			//csvファイルのデータ数
			model_rows[fileCount] = rows;
			//CSVファイル読み込み
			model_shape[fileCount] = csvread(model_filename[fileCount], model_rows[fileCount], cols);
			//コンソールにファイル名表示
			//cout << "model点群データファイル名 " << model_filename[fileCount] << endl;
		}
		///data
		//csvファイル名
		data_filename[fileCount] = (char *)malloc(sizeof(char *) * 100);
		//sprintf(data_filename[fileCount],"%s/%s/%d.csv",filedir,dir,(fileCount+1));
		sprintf(data_filename[fileCount],"%s/%s/points%02d.csv",filedir,dir,(fileCount+1));
		//csvファイルのデータ数
		data_rows[fileCount] = rows;
		//CSVファイル読み込み
		data_shape[fileCount] = csvread(data_filename[fileCount], data_rows[fileCount], cols);
		//コンソールにファイル名表示
		cout << "点群データファイル名 " << data_filename[fileCount] << endl;
#pragma endregion 

		if(fileCount>=1){

#pragma region // --- ICPによるレジストレーション ---
#if 1 // --- ICP実行する ---
			//実行時間計測開始
			start_time[fileCount] = clock();
			cout << "\t標準ICP開始" << endl;
			//ICP with flann search and unit quaternion method
			//cout << "kd-tree探索＋クォータニオンにより[R/t]を推定します" << endl << endl;
			ClosestPointFlann model_shape_flann (model_shape[fileCount]);
			RT_L2 <float, SolveRot_eigen<float>> rt_solver;
			ICP <ClosestPointFlann> icp (model_shape_flann, rt_solver);

			icp.set(data_shape[fileCount]);
			icp.reg(100, 1.0e-6);

			//実行時間計測終了
			end_time[fileCount] = clock();
			//cout << "icp result : [R/t] =" << endl << (icp.rt) << endl << endl;
			cout << "\t" << data_filename[fileCount] << "  icp error =" << icp.dk << endl;
			cout << "\t" << data_filename[fileCount] << "  実行時間 = " << (float)(end_time[fileCount] - start_time[fileCount])/CLOCKS_PER_SEC << "秒" << endl << endl;
			
			//データをローカル変数に格納
			//my_model_corr[fileCount] = Mat::zeros(rows, cols, CV_32F);
			my_model_corr[fileCount].create(rows, cols, CV_32F);
			icp.model_corr.copyTo(my_model_corr[fileCount]);
			icp.rt.copyTo(my_rt[fileCount]);
			for(int k=0;k<data_rows[fileCount];k++){
				myIndex[fileCount][k] = icp.index[k];
				myDist[fileCount][k] = icp.distance[k];
			}

#else // --- ICP実行しない場合 ---
			shape_reg[fileCount] = data_shape[fileCount];
#endif
#pragma endregion
		}else{
			shape_reg[fileCount] = data_shape[fileCount];
		}
	}

#pragma region // --- 座標変換 ---
	//平均値の計算
	reduce(shape_reg[0], model_mean, 0, CV_REDUCE_AVG);

#pragma omp parallel for private(i,j,k)
	for(fileCount=0;fileCount<fileTotal;fileCount++)
	{
		if(fileCount>=1){
			//得られたrtをdatashapeに適用
			//その前にshape_tempの初期化
			for(k=0;k<fileTotal;k++)
			{
				shape_temp[fileCount][k] = cv::Mat::zeros(data_rows[fileCount], cols, CV_32F);
			}
			shape_temp[fileCount][fileCount] = data_shape[fileCount];
			for(k=0;k<fileCount;k++)
			{
				shape_temp[fileCount][fileCount-(k+1)] = my_rt[(fileCount-k)].transform(shape_temp[fileCount][fileCount-k]);
			}
			shape_reg[fileCount] = shape_temp[fileCount][0];
		}

		shape_fixed[fileCount] = shape_reg[fileCount] - repeat(model_mean, shape_reg[fileCount].rows, 1);
		
		/*
		//メモリ割り当て
		points[fileCount] = (GLfloat *)malloc(sizeof(float)*data_rows[fileCount]*cols);
		//座標値をGLpointsに入れる
		for(i=0;i<data_rows[fileCount];i++){
			for(j=0;j<cols;j++){
				points[fileCount][i*cols+j] = shape_fixed[fileCount].at<float>(i,j);
			}
		}*/
#pragma endregion
	}

#pragma region // --- OpenGLにデータ渡す ---
	
	//メモリ割り当て
	allpoints = (GLfloat *)malloc(sizeof(float)*rows*fileTotal*cols);
	for(fileCount=0;fileCount<fileTotal;fileCount++)
	{
		//座標値をallpointsに入れる
		for(int i=0;i<rows;i++){
			for(int j=0;j<cols;j++){
				allpoints[fileCount*rows*cols+i*cols+j] = shape_fixed[fileCount].at<float>(i,j);
			}
		}
	}
#pragma endregion

	
#pragma region // --- カメラRTの計算 ---
	Mat cameraRT[fileTotal];
	Mat cameraR[fileTotal];
	Mat cameraT[fileTotal];
	cameraRT[0] = Mat::eye(4,4,CV_32F);
	cameraR[0] = Mat::eye(3,3,CV_32F);
	cameraT[0] = Mat::zeros(1,3,CV_32F);
	for(i=1;i<fileTotal;i++){
		cameraRT[i] = Mat::eye(4,4,CV_32F);
		cameraR[i] = Mat::eye(3,3,CV_32F);
		cameraT[i] = Mat::zeros(1,3,CV_32F);
		
		Mat r = my_rt[i].operator()(Range(0,3),Range(0,3));
		cameraR[i] = cameraR[i-1]*r.t();
		Mat t = my_rt[i].operator()(Range(3,4),Range(0,3));
		cameraT[i] = t*cameraR[i-1].t() + cameraT[i-1];
		
		cameraRT[i].at<float>(0,0) = cameraR[i].at<float>(0,0);
		cameraRT[i].at<float>(0,1) = cameraR[i].at<float>(0,1);
		cameraRT[i].at<float>(0,2) = cameraR[i].at<float>(0,2);
		cameraRT[i].at<float>(1,0) = cameraR[i].at<float>(1,0);
		cameraRT[i].at<float>(1,1) = cameraR[i].at<float>(1,1);
		cameraRT[i].at<float>(1,2) = cameraR[i].at<float>(1,2);
		cameraRT[i].at<float>(2,0) = cameraR[i].at<float>(2,0);
		cameraRT[i].at<float>(2,1) = cameraR[i].at<float>(2,1);
		cameraRT[i].at<float>(2,2) = cameraR[i].at<float>(2,2);
		
		cameraRT[i].at<float>(3,0) = cameraT[i].at<float>(0,0);
		cameraRT[i].at<float>(3,1) = cameraT[i].at<float>(0,1);
		cameraRT[i].at<float>(3,2) = cameraT[i].at<float>(0,2);
	}
#pragma endregion

// --- データ出力 ---
#if FILEOUTPUT

	///////////////////////////////
	// 全ての点群(shape_fixed)をまとめて書き出し
	// pcd
	//
	FILE *outfp;
	char outfilename[100];
	sprintf(outfilename,"%s/%s/result_xyz.pcd",outdir,dir);
	outfp = fopen(outfilename,"w");
	if(outfp == NULL){
		printf("%sファイルが開けません\n",outfilename);
		return -1;
	}
	int red = 255*256*256;
	int green = 255*256*256 + 255*256;
	int white = 255*256*256 + 255*256 + 255;
	fprintf(outfp,"# .PCD v.7 - Point Cloud Data file format\nVERSION .7\nFIELDS x y z rgb\nSIZE 4 4 4 4\nTYPE F F F F\nCOUNT 1 1 1 1\nWIDTH %d\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS %d\nDATA ascii\n", rows*fileTotal, rows*fileTotal);
	for(i=0;i<fileTotal;i++){
		for(j=0;j<data_rows[i];j++){
			fprintf(outfp,"%f %f %f %d\n", shape_reg[i].at<float>(j,0), shape_reg[i].at<float>(j,1), shape_reg[i].at<float>(j,2), green+(int)floor(255.*(i+1)/fileTotal));
		}
	}
	fclose(outfp);

	///////////////////////////////
	// 全ての点群(shape_fixed)をまとめて書き出し
	// csv
	//
	sprintf(outfilename,"%s/%s/allpoints.csv",outdir,dir);
	outfp = fopen(outfilename,"w");
	if(outfp == NULL){
		printf("%sファイルが開けません\n",outfilename);
		return -1;
	}
	for(i=0;i<fileTotal;i++){
		for(j=0;j<data_rows[i];j++){
			fprintf(outfp,"%f %f %f\n", shape_reg[i].at<float>(j,0), shape_reg[i].at<float>(j,1), shape_reg[i].at<float>(j,2));
		}
	}
	fclose(outfp);

	///////////////////////////////
	// 全ての点群(shape_fixed)をまとめて書き出し
	// result_xyz.csv
	//
	sprintf(outfilename,"%s/%s/result_xyz_icp.csv",outdir,dir);
	outfp = fopen(outfilename,"w");
	if(outfp == NULL){
		printf("%sファイルが開けません\n",outfilename);
		return -1;
	}
	for(i=0;i<fileTotal;i++){
		for(j=0;j<data_rows[i];j++){
			fprintf(outfp,"%f,%f,%f\n", shape_reg[i].at<float>(j,0), shape_reg[i].at<float>(j,1), shape_reg[i].at<float>(j,2));
		}
	}
	fclose(outfp);

	//////////////////////////////////
	// Corr(対応点), Index（対応点の要素番号）, Distance（対応点間距離）の書き出し
	//
	FILE *outfp_corr;
	char outfilename_corr[100];

	for(fileCount=1;fileCount<fileTotal;fileCount++){

		///Indexファイル
		sprintf(outfilename_corr,"%s/%s/index%02d.csv",outdir,dir,(fileCount));
		outfp_corr = fopen(outfilename_corr,"w");
		if(outfp_corr == NULL){
			printf("%sファイルが開けません\n",outfilename_corr);
			return -1;
		}
		for(j=0;j<data_rows[fileCount];j++){
			fprintf(outfp_corr,"%d\n", myIndex[fileCount][j]);
		}
		fclose(outfp_corr);

		///Distanceファイル
		sprintf(outfilename_corr,"%s/%s/dist%02d.csv",outdir,dir,(fileCount));
		outfp_corr = fopen(outfilename_corr,"w");
		if(outfp_corr == NULL){
			printf("%sファイルが開けません\n",outfilename_corr);
			return -1;
		}
		for(j=0;j<data_rows[fileCount];j++){
			fprintf(outfp_corr,"%f\n", myDist[fileCount][j]);
		}
		fclose(outfp_corr);
	}
	
	for(fileCount=0;fileCount<fileTotal;fileCount++){
		
		if(fileCount<(fileTotal-1)){
			///Corr点群ファイル
			sprintf(outfilename_corr,"%s/%s/corr%02d.csv",outdir,dir,(fileCount+1));
			outfp_corr = fopen(outfilename_corr,"w");
			if(outfp_corr == NULL){
				printf("%sファイルが開けません\n",outfilename_corr);
				return -1;
			}

			for(j=0;j<data_rows[fileCount];j++){
				//fprintf(outfp_corr,"%f %f %f\n", my_model_corr[fileCount].at<float>(j,0), my_model_corr[fileCount].at<float>(j,1), my_model_corr[fileCount].at<float>(j,2));
				fprintf(outfp_corr,"%f %f %f\n", shape_reg[fileCount].at<float>(myIndex[fileCount+1][j],0), shape_reg[fileCount].at<float>(myIndex[fileCount+1][j],1), shape_reg[fileCount].at<float>(myIndex[fileCount+1][j],2));
			}
			fclose(outfp_corr);
		}else{
			///Corr点群ファイル
			sprintf(outfilename_corr,"%s/%s/corr%02d.csv",outdir,dir,(fileCount+1));
			outfp_corr = fopen(outfilename_corr,"w");
			if(outfp_corr == NULL){
				printf("%sファイルが開けません\n",outfilename_corr);
				return -1;
			}

			for(j=0;j<data_rows[fileCount];j++){
				//fprintf(outfp_corr,"%f %f %f\n", my_model_corr[fileCount].at<float>(j,0), my_model_corr[fileCount].at<float>(j,1), my_model_corr[fileCount].at<float>(j,2));
				fprintf(outfp_corr,"%f %f %f\n", shape_reg[fileCount].at<float>(j,0), shape_reg[fileCount].at<float>(j,1), shape_reg[fileCount].at<float>(j,2));
			}
			fclose(outfp_corr);
		}
	}

	/////////////////////
	// RTの書き出し
	//
	//my_rt[0]に恒等変換を代入
	//Mat rt0 = (Mat_<float>(4,4) << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	Mat rt0 = Mat::eye(4,4,CV_32F);
	rt0.copyTo(my_rt[0]);
	// Open File Storage
	char rtfilename[100];
	sprintf(rtfilename,"%s/%s/rt.xml",outdir,dir);
	cv::FileStorage	cvfs(rtfilename,CV_STORAGE_WRITE);
	cv::WriteStructContext ws(cvfs, "mat_rt", CV_NODE_SEQ);	// create node
	for(int i=0; i<fileTotal; i++){
		cv::write(cvfs,"",cameraRT[i]);
	}
	cvfs.release();

#endif

//--- OpenGLで表示 ---
#if GLVIEW
	
	// --- GLUT initialize ---
	initFlag();
	initParam();

	//window1
	glutInit(&argc, argv);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(window_w, window_h);
	glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE );

	window1 = glutCreateWindow("Window1");
	glutMouseFunc(mouse);
	glutMotionFunc(drag);
	glutPassiveMotionFunc(passive);
	glutMouseWheelFunc ( MouseWheel ) ;//ホイールコールバック
	glutDisplayFunc(disp);
	glutIdleFunc(myGlutIdle);
	glutKeyboardFunc(glut_keyboard);
	glutIdleFunc(animate);
	glClearColor(0.0, 0.0, 0.0, 0.5); //背景色

	glutMainLoop();
#endif
	
	//実行時間計測終了
	end_time_total = clock();
	cout << "-------------" << endl;
	cout << "    Finish   " << endl;
	cout << "-------------" << endl;
	cout << "プログラム実行時間 = " << (float)(end_time_total - start_time_total)/CLOCKS_PER_SEC << "秒" << endl << endl;
	
	//cvNamedWindow ("WaitKey", CV_WINDOW_AUTOSIZE);
	//cvWaitKey(0);
	return 0;
}

// csvread : csvファイルをcv::Mat形式で出力
// filename : ファイル名, rows : 行数, cols : 列数
cv::Mat csvread(const char* filename, const int rows, const int cols){
	cv::Mat_<float> m;
	m = cv::Mat::zeros(rows, cols, CV_32F);

	// CSVファイルを開く
	ifstream ifs(filename);
	string str;
	// ファイルが存在しない場合
	if(ifs.fail()) {
		std::cout << "File does not exist" << std::endl;
		exit(0);
	}

	// 列数に合わせて適宜変数を指定
	float x,y,z;
	int line = 0;
	// 1行ずつ読んでいく
	while(getline(ifs, str)) {
		sscanf_s(str.data(), "%f %f %f", &x, &y, &z);
		m.at<float>(line,0) = x;
		m.at<float>(line,1) = y;
		m.at<float>(line,2) = z;
		line ++;
		if(line>=rows){
			break;
		}
	}
	return m;
}
