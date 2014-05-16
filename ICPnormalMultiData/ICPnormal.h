#ifndef ICP_NORMAL_H_INCLUDED
#define ICP_NORMAL_H_INCLUDED

#include <limits>

/*
//Matの要素をcoutで表示させる
//しかしこれは既にOpenCVで実装されているので、不要。
inline ostream& print(const Mat& A, ostream& stream_=cout)
{
	for(int i=0;i<A.rows;i++)
	{
		if(i) stream_ << endl;
		for(int j=0; j<A.cols; j++)
		{
			switch(A.type())
			{
			case CV_32F:
				stream_ << A.at<float>(i,j) << " ";
				break;
			case CV_64F:
				stream_ << A.at<double>(i,j) << " ";
				break;
				//define cases for other necessary types
			default:
				break;
			}
		}
	}
	return stream_;
}

ostream& operator<<(ostream& ost, const Mat& mat)
{
	return print(mat,ost);
}
*/

//LMS estimation of rotation matrix between point pairs by the unit quaternion representation
template<class T=float>
class SolveRot_eigen
{
public:
	Mat operator()(const Mat& cov)const
	{
		T tr_cov = trace(cov)[0];
		Mat qmat = Mat::diag(Mat_<T>(Vec<T,4>(1,-1,-1,-1)*tr_cov));
		Mat antisym(Mat_<T>(Vec<T,3>(
			cov.at<T>(1,2)-cov.at<T>(2,1),
			cov.at<T>(2,0)-cov.at<T>(0,2),
			cov.at<T>(0,1)-cov.at<T>(1,0))));
		Mat qmat1(qmat,Range(0,1),Range(1,4));
		qmat1 = antisym.t();
		Mat qmat2(qmat,Range(1,4),Range(0,1));
		qmat2 = antisym;
		Mat qmat3(qmat,Range(1,4),Range(1,4));
		qmat3 += cov+cov.t();
		Mat evals,evecs;
		eigen(qmat,evals,evecs,0,0);
		Mat evec1(evecs.row(0));
		T q0 = evecs.at<T>(0,0);
		T q1 = evecs.at<T>(0,1);
		T q2 = evecs.at<T>(0,2);
		T q3 = evecs.at<T>(0,3);
		Mat rot1 = (Mat_<T>(3,4) << 
			q0, q3, -q2, q1,
			-q3, q0, q1, q2,
			q2, -q1, q0, q3);
		Mat rot2 = (Mat_<T>(4,3) <<
			q0, q3, -q2,
			-q3, q0, q1,
			q2, -q1, q0,
			q1, q2, q3);
		Mat rot = rot1*rot2;
		return rot;
	}
};

//LMS estimation of rotation matrix between point pairs by SVD
template<class T=float>
class SolveRot_SVD
{
public:
	Mat operator()(const Mat& cov)const
	{
		SVD svd(cov, SVD::MODIFY_A);
		Mat sgn = Mat::diag(Mat_<T>(Vec<T,3>(1,1,determinant(svd.u*svd.vt))));
		Mat rot = svd.u*sgn*svd.vt;
		return rot;
	}
};

//3D Eulidean transform
template<class TYPE=float>
class RT: public Mat
{
public:
	RT(): Mat(Mat_<TYPE>::eye(4,4)){};
	RT(const Mat& mat): Mat(mat){}
	RT(TYPE angle, const Vec<TYPE,3>& axis, const Vec<TYPE,3>& translation)
		: Mat(Mat_<TYPE>::eye(4,4))
	{
		Mat src(Mat_<TYPE>(axis*TYPE(angle/norm(axis))));
		Mat rot;
		Rodrigues(src,rot);
		Mat r = operator()(Range(0,3),Range(0,3));
		rot.copyTo(r);
		Mat tt(Mat_<TYPE>(translation).t());
		Mat t = operator()(Range(3,4),Range(0,3));
		tt.copyTo(t);
	}

	//LMS estimation of Euclidean transform between point pairs
	template<class SolveRot>
	RT(const Mat& mat1, const Mat& mat2, const SolveRot& solve_rot)
		: Mat(Mat_<TYPE>::eye(4,4))
	{
		Mat_<TYPE> mean1;
		reduce(mat1, mean1, 0, CV_REDUCE_AVG);
		Mat_<TYPE> mat1d = mat1 - repeat(mean1, mat1.rows, 1);

		Mat_<TYPE> mean2;
		reduce(mat2, mean2, 0, CV_REDUCE_AVG);
		Mat_<TYPE> mat2d = mat2 - repeat(mean2, mat2.rows, 1);

		Mat cov = mat1d.t()*mat2d;
		Mat rot = solve_rot(cov);
		Mat trns = mean2 - mean1*rot;
		Mat r = operator()(Range(0,3),Range(0,3));
		rot.copyTo(r);
		Mat t = operator()(Range(3,4),Range(0,3));
		trns.copyTo(t);
	}

	//Weighted LMS estimation of Eucidean transform between point pairs
	template<class SolveRot>
	RT(const Mat& mat1, const Mat& mat2, const Mat& w, const SolveRot& solve_rot)
		: Mat (Mat_<TYPE>::eye(4,4))
	{
		TYPE sw = sum(w)[0];
		Mat_<TYPE> w1(w/sw);
		Mat_<TYPE> w3;
		repeat(w1,1,3,w3);

		Mat_<TYPE> mean1;
		reduce(mat1.mul(w3), mean1, 0, CV_REDUCE_SUM);
		Mat_<TYPE> mat1d = mat1 - repeat(maen1, mat1.rows, 1);

		Mat_<TYPE> mean2;
		reduce(mat2.mul(w3), mean2, 0, CV_REDUCE_SUM);
		Mat_<TYPE> mat2d = mat2 - repeat(maen2, mat2.rows, 1);

		Mat cov = mat1d.t()*(w3.mul(mat2d));
		Mat rot = solve_rot(cov);
		Mat trns = mean2 - mean1*rot;
		Mat r = operator()(Range(0,3),Range(0,3));
		rot.copyTo(r);
		Mat t = operator()(Range(3,4),Range(0,3));
		trns.copyTo(t);
	}
	Mat transform(const Mat& coords)
	{
		Mat r = operator()(Range(0,3),Range(0,3));
		Mat t = operator()(Range(3,4),Range(0,3));
		Mat transformed = coords*r + repeat(t,coords.rows,1);
		return transformed;
	}
};

 //LMS RT solver
template<class TYPE=float, class SOLVE_ROT=SolveRot_eigen<TYPE> >
class RT_L2
{
public:
	RT_L2(){}
	TYPE operator()(const Mat& data_shape, const Mat& model_corr, RT<TYPE>& rt)const
	{
		rt = RT<TYPE>(data_shape, model_corr, SOLVE_ROT());
		Mat_<TYPE> data_shape_rt = rt.transform(data_shape);
		Mat_<TYPE> dif = data_shape_rt - model_corr;
		Mat_<TYPE> e2;
		cv::reduce(dif.mul(dif), e2, 1, CV_REDUCE_SUM);
		TYPE rms = mean(e2)[0];
		return rms;
	}
};

template<class MODEL_SHAPE, class TYPE=float, class SOLVE_RT=RT_L2<TYPE,SolveRot_eigen<TYPE>>>
class ICP
{
	MODEL_SHAPE& _model_shape;
	SOLVE_RT _solve_rt;
public:
	RT<TYPE> rt0; //initial transform
	RT<TYPE> rt; //current transform
	Mat data_shape;
	Mat_<TYPE> data_shape_rt;
	Mat_<TYPE> model_corr;
	int iter; //current number of iterations
	TYPE dk;
	bool converged;
	
	//initialization
	ICP(MODEL_SHAPE& model_shape, const SOLVE_RT& solve_rt)
		: _model_shape(model_shape), _solve_rt(solve_rt){}

	//set data shape and initial transform
	void set(const Mat& data_shape_, const RT<TYPE> rt_ = RT<TYPE>())
	{
		data_shape_.copyTo(data_shape);
		rt_.copyTo(rt0);
		rt0.copyTo(rt);
		data_shape_rt = rt0.transform(data_shape);
		iter = 0;
		dk = numeric_limits<TYPE>::max();
		converged = false;
	}

	//registration
	bool reg(int max_iter=100, double tau=1.0e-6)
	{
		if(converged) return true;
		for(int iiter=0; iiter<max_iter; iiter++, iter++)
		{
			_model_shape.query(data_shape_rt, model_corr);
			TYPE dk1 = _solve_rt(data_shape, model_corr, rt);
			//cout << "estimated [R/t]:" << endl << rt << endl;
			data_shape_rt = rt.transform(data_shape);
			//cout << "d[" << iter << "]=" << dk1 << endl;
			dk = dk1;
		}
		return (converged=false);
	}
};

//naive closest point search
class ClosestPointNaive
{
	Mat _model_points;
public:
	ClosestPointNaive(const Mat& model_points): _model_points(model_points){}
	
	void query(const Mat& query_points, Mat& corr)
	{
		corr.create(query_points.rows, 3, query_points.type());
		for(int i=0; i<query_points.rows; i++)
		{
			Mat dif = _model_points - repeat(query_points.row(i), _model_points.rows, 1);
			Mat d2;
			reduce(dif.mul(dif), d2, 1, CV_REDUCE_SUM);
			Point minLoc;
			minMaxLoc(d2, NULL, NULL, &minLoc, NULL);
			Mat corr_i = corr.row(i);
			_model_points.row(minLoc.y).copyTo(corr_i);
		}
	}
};

//closest point search by kd-tree of flann
class ClosestPointFlann: public cv::flann::Index
{
	Mat _model_points;
public:
	ClosestPointFlann(const Mat& model_points): _model_points(model_points), cv::flann::Index(model_points, cv::flann::KDTreeIndexParams(1)){}

	void query(const Mat& query_points, Mat& corr)
	{
		int num = query_points.rows;
		Mat indices(num, 1, CV_32S);
		Mat dists(num, 1, CV_32F);
		cv::flann::SearchParams params(1024);
		knnSearch(query_points, indices, dists, 1, params);
		corr = Mat(num, _model_points.cols, _model_points.type());
		for(int i_row=0; i_row<num; i_row++)
		{
			Mat corr_i(corr.row(i_row));
			_model_points.row(indices.at<int>(i_row, 0)).copyTo(corr_i);
		}
	}
};

#endif ICP_NORMAL_H_INCLUDED